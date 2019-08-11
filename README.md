# SPIRV-Tools custom optimization passes

This is a collection of custom optimization passes for use with [SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools). They may or may not be merged into that project someday.

# Usage

Include the header file:

```cpp
#include "FuseMultiplyAddPass.h"
```

And then, in your optimizer invocation, add it to your pass registration chain:

```cpp
spvtools::Optimizer Opt(GTargetEnvironment);
Opt.RegisterPerformancePasses()
	.RegisterPass(std::make_unique<spvtools::Optimizer::PassToken::Impl>(std::make_unique<spvtools::opt::FuseMultiplyAddPass>()))
	.Run(Words, Count, &OptimizedSPIRV);
```

Please note that the implementation `.cpp` files require access to private sources of SPIRV-Tools, just like the original, internal optimizer passes.

# Passes

## [FuseMultiplyAddPass](FuseMultiplyAddPass.h)

Searches for `OpFMul`s depended on solely by `OpFAdd`s (or `OpFSub`s via `OpFNegate`, if this feature is enabled). If neither operand is decorated with `NoContraction`, it replaces the two with a single `GLSL.std.450` `Fma` extended instruction.

Does not support the OpenCL `fma` instruction, but can be trivially extended to do so.

# License

```
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
```
