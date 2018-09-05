target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%blockheader = type { i64 } 
%block = type { %blockheader, [0 x i64 *] } ; 16-bit layout, 8-bit length, 32-bit tag, children

declare %block* @parseConfiguration(i8*)
declare i32 @atoi(i8*)

declare void @take_steps(i32, %block*) #0

define i32 @main(i32 %argc, i8** %argv) {
entry:
  %filename_ptr = getelementptr inbounds i8*, i8** %argv, i64 1
  %filename = load i8*, i8** %filename_ptr
  %depth_ptr = getelementptr inbounds i8*, i8** %argv, i64 2
  %depth_str = load i8*, i8** %depth_ptr
  %depth = call i32 @atoi(i8* %depth_str)
  %ret = call %block* @parseConfiguration(i8* %filename)
  call void @take_steps(i32 %depth, %block* %ret)
  unreachable
}

attributes #0 = { noreturn }
