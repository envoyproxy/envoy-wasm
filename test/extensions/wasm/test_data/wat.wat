(module
  (type $0 (func (param i32 i32 i32)))
  (type $1 (func))
  (import "env" "_proxy_log" (func $_proxy_log (param i32 i32 i32)))
  (export "memory" (memory $2))
  (export "main" (func $main))
  (memory $2 17)
  (data $2 (i32.const 1048576) "Hello, world!")

 (func $main (type $1)
   i32.const 1
   i32.const 1048576
   i32.const 13
   call $_proxy_log
   )
 )
