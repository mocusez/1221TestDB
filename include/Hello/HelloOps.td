#ifndef HELLO_OPS
#define HELLO_OPS

include "HelloDialect.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

def ConstantOp : Hello_Op<"constant", [Pure]> {
  let summary = "constant";
  let description = [{
    Constant operation turns a literal into an SSA value. The data is attached
    to the operation as an attribute. For example:

    ```mlir
      %0 = "hello.constant"()
      { value = dense<[[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]]> : tensor<2x3xf64> }
      : () -> tensor<2x3xf64>
    ```
  }];

  let builders = [
    OpBuilder<(ins "mlir::DenseElementsAttr":$value), [{
      build($_builder, $_state, value.getType(), value);
    }]>,
    OpBuilder<(ins "double":$value)>
  ];

//  let parser = [{ return ::parseConstantOp(parser, result); }];
  let arguments = (ins F64ElementsAttr:$value);
  let results = (outs F64Tensor);
}

def PrintOp : Hello_Op<"print", [Pure]> {
    let summary = "print operation";
    let description = [{
        The "print" builtin operation prints a given input tensor, and produces
        no results.
    }];

    // The print operation takes an input tensor to print.
    let arguments = (ins AnyTypeOf<[F64Tensor, F64MemRef]>:$input);

    let assemblyFormat = "$input attr-dict `:` type($input)";
}

def WorldOp : Hello_Op<"world", [Pure]> {
    let summary = "print Hello, World";
    let description = [{
        The "world" operation prints "Hello, World", and produces
        no results.
    }];
}

def TestOp : Hello_Op<"test", [Pure]> {
    let summary = "print Testing,Testng!";
    let description = [{
        The "world" operation prints "Testing,Testng", and produces
        no results.
    }];
}

def AddOp : Hello_Op<"add", [Pure]> {
    let summary = "add two integers";
    let description = [{
        Adds two integer values and returns their sum.

        Example:
        ```mlir
        %result = hello.add %a, %b : i32
        ```
    }];

    // Define the operands
    let arguments = (ins I32:$lhs, I32:$rhs);
    
    // Define the results
    let results = (outs I32:$result);
    
    // Assembly format
    let assemblyFormat = "$lhs `,` $rhs attr-dict `:` type($result)";
}


#endif // HELLO_OPS
