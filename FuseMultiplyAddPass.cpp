/*
MIT License

Copyright (c) 2019 Leszek Godlewski

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "FuseMultiplyAddPass.h"

#include <unordered_set>
#include <vector>

#include "source/opt/decoration_manager.h"

namespace spvtools {
namespace opt {

Pass::Status FuseMultiplyAddPass::Process()
{
	bool Modified = false;

	for (Function& function : *get_module())
	{
		Modified |= ProcessFunction(&function);
	}
	return (Modified ? Status::SuccessWithChange : Status::SuccessWithoutChange);
}

static bool IsIncontractible(analysis::DecorationManager& DecMgr, const Instruction& Inst)
{
	return !DecMgr.WhileEachDecoration(Inst.result_id(), SpvDecorationNoContraction, [](const Instruction&) { return false; });
}

bool FuseMultiplyAddPass::ProcessFunction(Function* function)
{
	bool Modified = false;
	std::unordered_set<Instruction*> InstructionsToKill;
	
	const uint32_t GLSLInstSetId = context()->get_feature_mgr()->GetExtInstImportId_GLSLstd450();

	cfg()->ForEachBlockInReversePostOrder(function->entry().get(), [&](BasicBlock* Block)
	{
		std::vector<Instruction*> Users;
		for (Instruction* Inst = &*Block->begin(); Inst; Inst = Inst->NextNode())
		{
			if (Inst->opcode() != SpvOpFMul)
				continue;

			bool IsFusable = !IsIncontractible(*context()->get_decoration_mgr(), *Inst);

			if (!IsFusable)
				continue;
			
			context()->AnalyzeUses(Inst);
			Users.clear();
			get_def_use_mgr()->ForEachUser(Inst, [&IsFusable, &Users, this](Instruction* Use)
			{
				if (Use->opcode() != SpvOpFAdd || IsIncontractible(*context()->get_decoration_mgr(), *Use))
					IsFusable = false;
				else
					Users.push_back(Use);
			});
			IsFusable &= !Users.empty();
			
			if (!IsFusable)
				continue;
			
			Modified = true;
			for (auto& User : Users)
			{
				uint32_t COperand = User->GetSingleWordInOperand(0) == Inst->result_id() ? User->GetSingleWordInOperand(1) : User->GetSingleWordInOperand(0);
				Instruction::OperandList OpList
				{
					Operand{ SPV_OPERAND_TYPE_ID, { GLSLInstSetId } },
					Operand{ SPV_OPERAND_TYPE_EXTENSION_INSTRUCTION_NUMBER, { GLSLstd450Fma } },
					Operand{ SPV_OPERAND_TYPE_ID, { Inst->GetSingleWordInOperand(0) } },
					Operand{ SPV_OPERAND_TYPE_ID, { Inst->GetSingleWordInOperand(1) } },
					Operand{ SPV_OPERAND_TYPE_ID, { COperand } }
				};
				auto Fma = std::make_unique<Instruction>(context(), SpvOpExtInst, Inst->type_id(), TakeNextId(), OpList);
				get_def_use_mgr()->AnalyzeInstDefUse(&*Fma);
				context()->ReplaceAllUsesWith(User->result_id(), Fma->result_id());
				
				InstructionsToKill.insert(Inst);
				InstructionsToKill.insert(Users.begin(), Users.end());
				
				User->InsertBefore(std::move(Fma));
			}
		}
	});

	for (Instruction* Inst : InstructionsToKill)
		context()->KillInst(Inst);

	return Modified;
}

}	// namespace opt
}	// namespace spvtools
