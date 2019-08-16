(module
  (type $0 (func (param i32 i32 i32) (result i32)))
  (type $1 (func (param i32 f64 i32 i32 i32 i32) (result i32)))
  (type $2 (func (param i32 i32)))
  (type $3 (func))
  (type $4 (func (param i32 i32 i32 i32)))
  (type $5 (func (param i32 i32 i32 i32 i32 i32)))
  (type $6 (func (param i32 i32 i32 i32 i32)))
  (type $7 (func (param i32)))
  (type $8 (func (result i32)))
  (type $9 (func (param i32) (result i32)))
  (type $10 (func (param i32 i32 i32)))
  (type $11 (func (param f64 i32) (result f64)))
  (type $12 (func (param i32 i32) (result i32)))
  (type $13 (func (param i32 i32 i32 i32) (result i32)))
  (type $14 (func (param i64 i32 i32) (result i32)))
  (type $15 (func (param i64 i32) (result i32)))
  (type $16 (func (param i32 i32 f64 i32 i32 i32 i32) (result i32)))
  (type $17 (func (param i32 i32 i32 i32 i32 i32 i32)))
  (import "env" "abort" (func $abort (param i32)))
  (import "env" "___cxa_uncaught_exceptions" (func $___cxa_uncaught_exceptions (result i32)))
  (import "env" "___setErrNo" (func $___setErrNo (param i32)))
  (import "env" "_abort" (func $_abort))
  (import "env" "_emscripten_get_heap_size" (func $_emscripten_get_heap_size (result i32)))
  (import "env" "_emscripten_memcpy_big" (func $_emscripten_memcpy_big (param i32 i32 i32) (result i32)))
  (import "env" "_emscripten_resize_heap" (func $_emscripten_resize_heap (param i32) (result i32)))
  (import "env" "_llvm_trap" (func $_llvm_trap))
  (import "env" "_proxy_log" (func $_proxy_log (param i32 i32 i32) (result i32)))
  (import "env" "abortOnCannotGrowMemory" (func $abortOnCannotGrowMemory (param i32) (result i32)))
  (import "env" "__table_base" (global $20 i32))
  (import "env" "DYNAMICTOP_PTR" (global $21 i32))
  (import "env" "memory" (memory $19 256 256))
  (import "env" "table" (table $18 29 29 funcref))
  (export "__GLOBAL__sub_I_status_cc" (func $__GLOBAL__sub_I_status_cc))
  (export "__ZSt18uncaught_exceptionv" (func $__ZSt18uncaught_exceptionv))
  (export "___cxa_can_catch" (func $___cxa_can_catch))
  (export "___cxa_is_pointer_type" (func $___cxa_is_pointer_type))
  (export "___errno_location" (func $___errno_location))
  (export "_free" (func $_free))
  (export "_malloc" (func $_malloc))
  (export "_memcpy" (func $_memcpy))
  (export "_memmove" (func $_memmove))
  (export "_memset" (func $_memset))
  (export "_proxy_onLog" (func $_proxy_onLog))
  (export "_proxy_onStart" (func $_proxy_onStart))
  (export "_sbrk" (func $_sbrk))
  (export "dynCall_iidiiii" (func $dynCall_iidiiii))
  (export "dynCall_iiii" (func $dynCall_iiii))
  (export "dynCall_v" (func $dynCall_v))
  (export "dynCall_vi" (func $dynCall_vi))
  (export "dynCall_vii" (func $dynCall_vii))
  (export "dynCall_viiii" (func $dynCall_viiii))
  (export "dynCall_viiiii" (func $dynCall_viiiii))
  (export "dynCall_viiiiii" (func $dynCall_viiiiii))
  (export "establishStackSpace" (func $establishStackSpace))
  (export "stackAlloc" (func $stackAlloc))
  (export "stackRestore" (func $stackRestore))
  (export "stackSave" (func $stackSave))
  (global $22  (mut i32) (i32.const 4240))
  (global $23  (mut i32) (i32.const 5247120))
  (elem $24 $18 (global.get $20)
    $b0 $_fmt_fp $b1 $_sn_write $__ZNK10__cxxabiv117__class_type_info9can_catchEPKNS_16__shim_type_infoERPv $b1 $b2 $b3
    $__ZN10__cxxabiv116__shim_type_infoD2Ev $__ZN10__cxxabiv117__class_type_infoD0Ev $__ZN10__cxxabiv116__shim_type_infoD2Ev $__ZN10__cxxabiv116__shim_type_infoD2Ev $__ZN10__cxxabiv117__class_type_infoD0Ev $b3 $b3 $b4
    $_pop_arg_long_double $b5 $__ZNK10__cxxabiv117__class_type_info27has_unambiguous_public_baseEPNS_19__dynamic_cast_infoEPvi $__ZNK10__cxxabiv120__si_class_type_info27has_unambiguous_public_baseEPNS_19__dynamic_cast_infoEPvi $b5 $b6 $__ZNK10__cxxabiv117__class_type_info16search_below_dstEPNS_19__dynamic_cast_infoEPKvib $__ZNK10__cxxabiv120__si_class_type_info16search_below_dstEPNS_19__dynamic_cast_infoEPKvib
    $b6 $b7 $__ZNK10__cxxabiv117__class_type_info16search_above_dstEPNS_19__dynamic_cast_infoEPKvS4_ib $__ZNK10__cxxabiv120__si_class_type_info16search_above_dstEPNS_19__dynamic_cast_infoEPKvS4_ib $b7)
  (data $25 $19 (i32.const 1024)
    "\11\00\n\00\11\11\11\00\00\00\00\05\00\00\00\00\00\00\09\00\00\00\00\0b")
  (data $26 $19 (i32.const 1056)
    "\11\00\0f\n\11\11\11\03\n\07\00\01\13\09\0b\0b\00\00\09\06\0b\00\00\0b\00\06\11\00\00\00\11\11\11")
  (data $27 $19 (i32.const 1105)
    "\0b")
  (data $28 $19 (i32.const 1114)
    "\11\00\n\n\11\11\11\00\n\00\00\02\00\09\0b\00\00\00\09\00\0b\00\00\0b")
  (data $29 $19 (i32.const 1163)
    "\0c")
  (data $30 $19 (i32.const 1175)
    "\0c\00\00\00\00\0c\00\00\00\00\09\0c\00\00\00\00\00\0c\00\00\0c")
  (data $31 $19 (i32.const 1221)
    "\0e")
  (data $32 $19 (i32.const 1233)
    "\0d\00\00\00\04\0d\00\00\00\00\09\0e\00\00\00\00\00\0e\00\00\0e")
  (data $33 $19 (i32.const 1279)
    "\10")
  (data $34 $19 (i32.const 1291)
    "\0f\00\00\00\00\0f\00\00\00\00\09\10\00\00\00\00\00\10\00\00\10\00\00\12\00\00\00\12\12\12")
  (data $35 $19 (i32.const 1346)
    "\12\00\00\00\12\12\12\00\00\00\00\00\00\09")
  (data $36 $19 (i32.const 1395)
    "\0b")
  (data $37 $19 (i32.const 1407)
    "\n\00\00\00\00\n\00\00\00\00\09\0b\00\00\00\00\00\0b\00\00\0b")
  (data $38 $19 (i32.const 1453)
    "\0c")
  (data $39 $19 (i32.const 1465)
    "\0c\00\00\00\00\0c\00\00\00\00\09\0c\00\00\00\00\00\0c\00\00\0c\00\000123456789ABCDEF")
  (data $40 $19 (i32.const 1540)
    "\01")
  (data $41 $19 (i32.const 1579)
    "\ff\ff\ff\ff\ff")
  (data $42 $19 (i32.const 1648)
    "\ec\07\00\00\dc\08\00\00\80\06\00\00\00\00\00\00\ec\07\00\00\89\08\00\00\90\06\00\00\00\00\00\00\c4\07\00\00\aa\08\00\00\ec\07\00\00\b7\08\00\00p\06\00\00\00\00\00\00\ec\07\00\00\fe\08\00\00"
    "\80\06\00\00\00\00\00\00\ec\07\00\00 \09\00\00\a8\06")
  (data $43 $19 (i32.const 1924)
    "\a8\09")
  (data $44 $19 (i32.const 1984)
    "p\06\00\00\01\00\00\00\02\00\00\00\03\00\00\00\04\00\00\00\02\00\00\00\01\00\00\00\01\00\00\00\01\00\00\00\00\00\00\00\98\06\00\00\01\00\00\00\05\00\00\00\03\00\00\00\04\00\00\00\02\00\00\00"
    "\02\00\00\00\02\00\00\00\02\00\00\00before badptr\00before div by zero\00divide by zero: \00af"
    "ter div by zero\00-+   0X0x\00(null)\00-0X+0X 0X-0x+0x 0x\00inf\00INF\00nan\00"
    "NAN\00.\00%u\00N10__cxxabiv116__shim_type_infoE\00St9type_info\00N10__cxxa"
    "biv120__si_class_type_infoE\00N10__cxxabiv117__class_type_infoE\00N1"
    "0__cxxabiv117__pbase_type_infoE\00N10__cxxabiv119__pointer_type_in"
    "foE")
  
  (func $stackAlloc (type $9)
    (param $0 i32)
    (result i32)
    (local $1 i32)
    global.get $22
    local.set $1
    local.get $0
    global.get $22
    i32.add
    global.set $22
    global.get $22
    i32.const 15
    i32.add
    i32.const -16
    i32.and
    global.set $22
    local.get $1
    )
  
  (func $stackSave (type $8)
    (result i32)
    global.get $22
    )
  
  (func $stackRestore (type $7)
    (param $0 i32)
    local.get $0
    global.set $22
    )
  
  (func $establishStackSpace (type $2)
    (param $0 i32)
    (param $1 i32)
    local.get $0
    global.set $22
    local.get $1
    global.set $23
    )
  
  (func $_proxy_onStart (type $6)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (param $4 i32)
    i32.const 4
    i32.const 2060
    i32.const 13
    call $_proxy_log
    drop
    call $_llvm_trap
    )
  
  (func $_proxy_onLog (type $7)
    (param $0 i32)
    (local $1 i32)
    (local $2 i32)
    (local $3 i32)
    global.get $22
    local.set $1
    global.get $22
    i32.const 32
    i32.add
    global.set $22
    local.get $1
    i32.const 12
    i32.add
    local.set $2
    i32.const 4
    i32.const 2074
    i32.const 18
    call $_proxy_log
    drop
    local.get $2
    local.get $0
    if $if (result i32)
      i32.const 100
      local.get $0
      i32.div_u
    else
      i32.const 0
    end ;; $if
    call $__ZNSt3__29to_stringEj
    local.get $1
    local.get $2
    call $_strlen
    call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6insertEmPKcm
    local.tee $0
    i64.load align=4
    i64.store align=4
    local.get $1
    local.get $0
    i32.load offset=8
    i32.store offset=8
    local.get $0
    i64.const 0
    i64.store align=4
    local.get $0
    i32.const 0
    i32.store offset=8
    i32.const 4
    local.get $1
    i32.load
    local.get $1
    local.get $1
    i32.load8_s offset=11
    local.tee $0
    i32.const 0
    i32.lt_s
    local.tee $3
    select
    local.get $1
    i32.load offset=4
    local.get $0
    i32.const 255
    i32.and
    local.get $3
    select
    call $_proxy_log
    drop
    local.get $1
    i32.load8_s offset=11
    i32.const 0
    i32.lt_s
    if $if_0
      local.get $1
      i32.load
      call $_free
    end ;; $if_0
    local.get $2
    i32.load8_s offset=11
    i32.const 0
    i32.ge_s
    if $if_1
      i32.const 4
      i32.const 2110
      i32.const 17
      call $_proxy_log
      drop
      local.get $1
      global.set $22
      return
    end ;; $if_1
    local.get $2
    i32.load
    call $_free
    i32.const 4
    i32.const 2110
    i32.const 17
    call $_proxy_log
    drop
    local.get $1
    global.set $22
    )
  
  (func $__GLOBAL__sub_I_status_cc (type $3)
    (local $0 i32)
    (local $1 i32)
    global.get $22
    local.set $0
    global.get $22
    i32.const 16
    i32.add
    global.set $22
    i32.const 2384
    i64.const 0
    i64.store align=4
    i32.const 2392
    i64.const 0
    i64.store align=4
    local.get $0
    i32.const 2996
    i32.store
    local.get $0
    i32.const 0
    i32.store offset=4
    local.get $0
    i32.const 8
    i32.add
    local.tee $1
    local.get $0
    i64.load align=4
    i64.store align=4
    i32.const 2400
    i32.const 1
    local.get $1
    call $__ZN6google8protobuf4util6StatusC2ENS1_5error4CodeENS0_11StringPieceE
    local.get $0
    i32.const 2996
    i32.store
    local.get $0
    i32.const 0
    i32.store offset=4
    local.get $1
    local.get $0
    i64.load align=4
    i64.store align=4
    i32.const 2416
    i32.const 2
    local.get $1
    call $__ZN6google8protobuf4util6StatusC2ENS1_5error4CodeENS0_11StringPieceE
    local.get $0
    global.set $22
    )
  
  (func $__ZN6google8protobuf4util6StatusC2ENS1_5error4CodeENS0_11StringPieceE (type $10)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    (local $7 i32)
    global.get $22
    local.set $5
    global.get $22
    i32.const 16
    i32.add
    global.set $22
    local.get $0
    local.get $1
    i32.store
    local.get $0
    i32.const 4
    i32.add
    local.tee $3
    i64.const 0
    i64.store align=4
    local.get $3
    i32.const 0
    i32.store offset=8
    local.get $1
    i32.eqz
    if $if
      local.get $5
      global.set $22
      return
    end ;; $if
    local.get $5
    local.set $1
    block $block
      block $block_0
        local.get $2
        i32.load
        local.tee $6
        if $if_0
          local.get $2
          i32.load offset=4
          local.set $4
          local.get $1
          i64.const 0
          i64.store align=4
          local.get $1
          i32.const 0
          i32.store offset=8
          local.get $4
          i32.const -17
          i32.gt_u
          if $if_1
            call $_abort
          end ;; $if_1
          block $block_1
            block $block_2
              local.get $4
              i32.const 11
              i32.lt_u
              if $if_2 (result i32)
                local.get $1
                local.get $4
                i32.store8 offset=11
                local.get $4
                if $if_3 (result i32)
                  local.get $1
                  local.set $2
                  br $block_2
                else
                  local.get $1
                end ;; $if_3
              else
                local.get $1
                local.get $4
                i32.const 16
                i32.add
                i32.const -16
                i32.and
                local.tee $7
                call $__Znwm
                local.tee $2
                i32.store
                local.get $1
                local.get $7
                i32.const -2147483648
                i32.or
                i32.store offset=8
                local.get $1
                local.get $4
                i32.store offset=4
                br $block_2
              end ;; $if_2
              local.set $2
              br $block_1
            end ;; $block_2
            local.get $2
            local.get $6
            local.get $4
            call $_memcpy
            drop
          end ;; $block_1
          local.get $2
          local.get $4
          i32.add
          i32.const 0
          i32.store8
          local.get $3
          i32.const 11
          i32.add
          local.tee $2
          i32.load8_s
          i32.const 0
          i32.lt_s
          if $if_4
            local.get $3
            i32.load
            i32.const 0
            i32.store8
            local.get $0
            i32.const 0
            i32.store offset=8
          else
            local.get $2
            local.set $0
            br $block_0
          end ;; $if_4
        else
          local.get $1
          i64.const 0
          i64.store align=4
          local.get $1
          i32.const 0
          i32.store offset=8
          local.get $3
          i32.const 11
          i32.add
          local.set $0
          br $block_0
        end ;; $if_0
        br $block
      end ;; $block_0
      local.get $3
      i32.const 0
      i32.store8
      local.get $0
      i32.const 0
      i32.store8
    end ;; $block
    local.get $3
    call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE7reserveEm
    local.get $3
    local.get $1
    i64.load align=4
    i64.store align=4
    local.get $3
    local.get $1
    i32.load offset=8
    i32.store offset=8
    local.get $5
    global.set $22
    )
  
  (func $___errno_location (type $8)
    (result i32)
    i32.const 2496
    )
  
  (func $_strlen (type $8)
    (result i32)
    (local $0 i32)
    (local $1 i32)
    (local $2 i32)
    i32.const 2093
    local.set $0
    block $block
      block $block_0
        i32.const 2093
        local.set $1
        loop $loop
          block $block_1
            local.get $0
            i32.load8_s
            i32.eqz
            if $if
              local.get $1
              local.set $0
              br $block_1
            end ;; $if
            local.get $0
            i32.const 1
            i32.add
            local.tee $0
            local.tee $1
            i32.const 3
            i32.and
            br_if $loop
            br $block_0
          end ;; $block_1
        end ;; $loop
        br $block
      end ;; $block_0
      loop $loop_0
        local.get $0
        i32.const 4
        i32.add
        local.set $1
        local.get $0
        i32.load
        local.tee $2
        i32.const -2139062144
        i32.and
        i32.const -2139062144
        i32.xor
        local.get $2
        i32.const -16843009
        i32.add
        i32.and
        i32.eqz
        if $if_0
          local.get $1
          local.set $0
          br $loop_0
        end ;; $if_0
      end ;; $loop_0
      local.get $2
      i32.const 255
      i32.and
      if $if_1
        loop $loop_1
          local.get $0
          i32.const 1
          i32.add
          local.tee $0
          i32.load8_s
          br_if $loop_1
        end ;; $loop_1
      end ;; $if_1
    end ;; $block
    local.get $0
    i32.const 2093
    i32.sub
    )
  
  (func $_frexp (type $11)
    (param $0 f64)
    (param $1 i32)
    (result f64)
    (local $2 i32)
    (local $3 i64)
    (local $4 i64)
    block $block
      block $block_0
        local.get $0
        i64.reinterpret_f64
        local.tee $3
        i64.const 52
        i64.shr_u
        local.tee $4
        i32.wrap_i64
        i32.const 2047
        i32.and
        local.tee $2
        if $if
          local.get $2
          i32.const 2047
          i32.eq
          if $if_0
            br $block
          else
            br $block_0
          end ;; $if_0
          unreachable
        end ;; $if
        local.get $1
        local.get $0
        f64.const 0x0.0000000000000p-1023
        f64.ne
        if $if_1 (result i32)
          local.get $0
          f64.const 0x1.0000000000000p+64
          f64.mul
          local.get $1
          call $_frexp
          local.set $0
          local.get $1
          i32.load
          i32.const -64
          i32.add
        else
          i32.const 0
        end ;; $if_1
        i32.store
        br $block
      end ;; $block_0
      local.get $1
      local.get $4
      i32.wrap_i64
      i32.const 2047
      i32.and
      i32.const -1022
      i32.add
      i32.store
      local.get $3
      i64.const -9218868437227405313
      i64.and
      i64.const 4602678819172646912
      i64.or
      f64.reinterpret_i64
      local.set $0
    end ;; $block
    local.get $0
    )
  
  (func $_strcmp (type $12)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    local.get $0
    i32.load8_s
    local.tee $2
    local.get $1
    i32.load8_s
    local.tee $3
    i32.ne
    local.get $2
    i32.eqz
    i32.or
    if $if (result i32)
      local.get $2
      local.set $1
      local.get $3
    else
      loop $loop (result i32)
        local.get $0
        i32.const 1
        i32.add
        local.tee $0
        i32.load8_s
        local.tee $2
        local.get $1
        i32.const 1
        i32.add
        local.tee $1
        i32.load8_s
        local.tee $3
        i32.ne
        local.get $2
        i32.eqz
        i32.or
        if $if_0 (result i32)
          local.get $2
          local.set $1
          local.get $3
        else
          br $loop
        end ;; $if_0
      end ;; $loop
    end ;; $if
    local.set $0
    local.get $1
    i32.const 255
    i32.and
    local.get $0
    i32.const 255
    i32.and
    i32.sub
    )
  
  (func $_memchr (type $12)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    block $block
      block $block_0
        block $block_1
          local.get $1
          i32.const 0
          i32.ne
          local.tee $2
          local.get $0
          i32.const 3
          i32.and
          i32.const 0
          i32.ne
          i32.and
          if $if
            loop $loop
              local.get $0
              i32.load8_u
              i32.eqz
              br_if $block_1
              local.get $1
              i32.const -1
              i32.add
              local.tee $1
              i32.const 0
              i32.ne
              local.tee $2
              local.get $0
              i32.const 1
              i32.add
              local.tee $0
              i32.const 3
              i32.and
              i32.const 0
              i32.ne
              i32.and
              br_if $loop
            end ;; $loop
          end ;; $if
          local.get $2
          i32.eqz
          br_if $block_0
        end ;; $block_1
        local.get $0
        i32.load8_u
        i32.eqz
        if $if_0
          local.get $1
          i32.eqz
          br_if $block_0
          br $block
        end ;; $if_0
        block $block_2
          block $block_3
            local.get $1
            i32.const 3
            i32.le_u
            br_if $block_3
            loop $loop_0
              local.get $0
              i32.load
              local.tee $2
              i32.const -2139062144
              i32.and
              i32.const -2139062144
              i32.xor
              local.get $2
              i32.const -16843009
              i32.add
              i32.and
              i32.eqz
              if $if_1
                local.get $0
                i32.const 4
                i32.add
                local.set $0
                local.get $1
                i32.const -4
                i32.add
                local.tee $1
                i32.const 3
                i32.gt_u
                br_if $loop_0
                br $block_3
              end ;; $if_1
            end ;; $loop_0
            br $block_2
          end ;; $block_3
          local.get $1
          i32.eqz
          br_if $block_0
        end ;; $block_2
        loop $loop_1
          local.get $0
          i32.load8_u
          i32.eqz
          br_if $block
          local.get $1
          i32.const -1
          i32.add
          local.tee $1
          i32.eqz
          br_if $block_0
          local.get $0
          i32.const 1
          i32.add
          local.set $0
          br $loop_1
          unreachable
        end ;; $loop_1
        unreachable
      end ;; $block_0
      i32.const 0
      local.set $0
    end ;; $block
    local.get $0
    )
  
  (func $_snprintf (type $0)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    (local $3 i32)
    global.get $22
    local.set $3
    global.get $22
    i32.const 16
    i32.add
    global.set $22
    local.get $3
    local.get $2
    i32.store
    local.get $0
    local.get $1
    local.get $3
    call $_vsnprintf
    local.set $0
    local.get $3
    global.set $22
    local.get $0
    )
  
  (func $_vsnprintf (type $0)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    (local $3 i32)
    (local $4 i32)
    global.get $22
    local.set $3
    global.get $22
    i32.const 160
    i32.add
    global.set $22
    local.get $3
    i32.const 144
    i32.add
    local.set $4
    local.get $3
    i32.const 1504
    i32.const 144
    call $_memcpy
    drop
    block $block
      block $block_0
        local.get $1
        i32.const -1
        i32.add
        i32.const 2147483646
        i32.le_u
        br_if $block_0
        local.get $1
        if $if (result i32)
          i32.const 2496
          i32.const 75
          i32.store
          i32.const -1
        else
          i32.const 1
          local.set $1
          local.get $4
          local.set $0
          br $block_0
        end ;; $if
        local.set $0
        br $block
      end ;; $block_0
      local.get $3
      i32.const -2
      local.get $0
      i32.sub
      local.tee $4
      local.get $1
      local.get $1
      local.get $4
      i32.gt_u
      select
      local.tee $1
      i32.store offset=48
      local.get $3
      local.get $0
      i32.store offset=20
      local.get $3
      local.get $0
      i32.store offset=44
      local.get $3
      local.get $0
      local.get $1
      i32.add
      local.tee $0
      i32.store offset=16
      local.get $3
      local.get $0
      i32.store offset=28
      local.get $3
      local.get $2
      call $___vfprintf_internal
      local.set $0
      local.get $1
      if $if_0
        local.get $3
        i32.load offset=20
        local.tee $1
        local.get $1
        local.get $3
        i32.load offset=16
        i32.eq
        i32.const 31
        i32.shl
        i32.const 31
        i32.shr_s
        i32.add
        i32.const 0
        i32.store8
      end ;; $if_0
    end ;; $block
    local.get $3
    global.set $22
    local.get $0
    )
  
  (func $_fmt_fp (type $1)
    (param $0 i32)
    (param $1 f64)
    (param $2 i32)
    (param $3 i32)
    (param $4 i32)
    (param $5 i32)
    (result i32)
    (local $6 i32)
    (local $7 i32)
    (local $8 i32)
    (local $9 i32)
    (local $10 i32)
    (local $11 i32)
    (local $12 i32)
    (local $13 i32)
    (local $14 i32)
    (local $15 i32)
    (local $16 i32)
    (local $17 i32)
    (local $18 i32)
    (local $19 i32)
    (local $20 i32)
    (local $21 i32)
    (local $22 i32)
    (local $23 i32)
    (local $24 i32)
    (local $25 i64)
    (local $26 i64)
    (local $27 i64)
    (local $28 f64)
    global.get $22
    local.set $21
    global.get $22
    i32.const 560
    i32.add
    global.set $22
    local.get $21
    i32.const 32
    i32.add
    local.set $7
    local.get $21
    local.tee $12
    local.set $17
    local.get $12
    i32.const 536
    i32.add
    local.tee $8
    i32.const 0
    i32.store
    local.get $12
    i32.const 540
    i32.add
    local.tee $10
    i32.const 12
    i32.add
    local.set $16
    local.get $1
    i64.reinterpret_f64
    local.tee $25
    i64.const 0
    i64.lt_s
    if $if (result i32)
      local.get $1
      f64.neg
      local.tee $1
      i64.reinterpret_f64
      local.set $25
      i32.const 2145
      local.set $18
      i32.const 1
    else
      i32.const 2148
      i32.const 2151
      i32.const 2146
      local.get $4
      i32.const 1
      i32.and
      select
      local.get $4
      i32.const 2048
      i32.and
      select
      local.set $18
      local.get $4
      i32.const 2049
      i32.and
      i32.const 0
      i32.ne
    end ;; $if
    local.set $19
    local.get $25
    i64.const 9218868437227405312
    i64.and
    i64.const 9218868437227405312
    i64.eq
    if $if_0 (result i32)
      i32.const 2172
      i32.const 2176
      local.get $5
      i32.const 32
      i32.and
      i32.const 0
      i32.ne
      local.tee $3
      select
      i32.const 2164
      i32.const 2168
      local.get $3
      select
      local.get $1
      local.get $1
      f64.ne
      select
      local.set $5
      local.get $0
      i32.const 32
      local.get $2
      local.get $19
      i32.const 3
      i32.add
      local.tee $3
      local.get $4
      i32.const -65537
      i32.and
      call $_pad_524
      local.get $0
      local.get $18
      local.get $19
      call $_out
      local.get $0
      local.get $5
      i32.const 3
      call $_out
      local.get $0
      i32.const 32
      local.get $2
      local.get $3
      local.get $4
      i32.const 8192
      i32.xor
      call $_pad_524
      local.get $3
    else
      block $block (result i32)
        local.get $1
        local.get $8
        call $_frexp
        f64.const 0x1.0000000000000p+1
        f64.mul
        local.tee $1
        f64.const 0x0.0000000000000p-1023
        f64.ne
        local.tee $6
        if $if_1
          local.get $8
          local.get $8
          i32.load
          i32.const -1
          i32.add
          i32.store
        end ;; $if_1
        local.get $5
        i32.const 32
        i32.or
        local.tee $13
        i32.const 97
        i32.eq
        if $if_2
          local.get $18
          i32.const 9
          i32.add
          local.get $18
          local.get $5
          i32.const 32
          i32.and
          local.tee $14
          select
          local.set $9
          i32.const 12
          local.get $3
          i32.sub
          local.tee $7
          i32.eqz
          local.get $3
          i32.const 11
          i32.gt_u
          i32.or
          i32.eqz
          if $if_3
            f64.const 0x1.0000000000000p+3
            local.set $28
            loop $loop
              local.get $28
              f64.const 0x1.0000000000000p+4
              f64.mul
              local.set $28
              local.get $7
              i32.const -1
              i32.add
              local.tee $7
              br_if $loop
            end ;; $loop
            local.get $9
            i32.load8_s
            i32.const 45
            i32.eq
            if $if_4 (result f64)
              local.get $28
              local.get $1
              f64.neg
              local.get $28
              f64.sub
              f64.add
              f64.neg
            else
              local.get $1
              local.get $28
              f64.add
              local.get $28
              f64.sub
            end ;; $if_4
            local.set $1
          end ;; $if_3
          local.get $16
          i32.const 0
          local.get $8
          i32.load
          local.tee $6
          i32.sub
          local.get $6
          local.get $6
          i32.const 0
          i32.lt_s
          select
          i64.extend_i32_s
          local.get $16
          call $_fmt_u
          local.tee $7
          i32.eq
          if $if_5
            local.get $10
            i32.const 11
            i32.add
            local.tee $7
            i32.const 48
            i32.store8
          end ;; $if_5
          local.get $19
          i32.const 2
          i32.or
          local.set $8
          local.get $7
          i32.const -1
          i32.add
          local.get $6
          i32.const 31
          i32.shr_s
          i32.const 2
          i32.and
          i32.const 43
          i32.add
          i32.store8
          local.get $7
          i32.const -2
          i32.add
          local.tee $6
          local.get $5
          i32.const 15
          i32.add
          i32.store8
          local.get $3
          i32.const 1
          i32.lt_s
          local.set $10
          local.get $4
          i32.const 8
          i32.and
          i32.eqz
          local.set $11
          local.get $12
          local.set $5
          loop $loop_0
            local.get $5
            local.get $14
            local.get $1
            local.get $1
            f64.ne
            if $if_6 (result i32)
              i32.const -2147483648
            else
              local.get $1
              f64.const 0x1.0000000000000p+31
              f64.ge
              if $if_7 (result i32)
                i32.const -2147483648
              else
                local.get $1
                f64.const -0x1.0000000200000p+31
                f64.le
                if $if_8 (result i32)
                  i32.const -2147483648
                else
                  local.get $1
                  i32.trunc_f64_s
                end ;; $if_8
              end ;; $if_7
            end ;; $if_6
            local.tee $7
            i32.const 1488
            i32.add
            i32.load8_u
            i32.or
            i32.store8
            local.get $1
            local.get $7
            f64.convert_i32_s
            f64.sub
            f64.const 0x1.0000000000000p+4
            f64.mul
            local.set $1
            local.get $5
            i32.const 1
            i32.add
            local.tee $7
            local.get $17
            i32.sub
            i32.const 1
            i32.eq
            if $if_9 (result i32)
              local.get $10
              local.get $1
              f64.const 0x0.0000000000000p-1023
              f64.eq
              i32.and
              local.get $11
              i32.and
              if $if_10 (result i32)
                local.get $7
              else
                local.get $7
                i32.const 46
                i32.store8
                local.get $5
                i32.const 2
                i32.add
              end ;; $if_10
            else
              local.get $7
            end ;; $if_9
            local.set $5
            local.get $1
            f64.const 0x0.0000000000000p-1023
            f64.ne
            br_if $loop_0
          end ;; $loop_0
          block $block_0 (result i32)
            block $block_1
              local.get $3
              i32.eqz
              br_if $block_1
              local.get $5
              i32.const -2
              local.get $17
              i32.sub
              i32.add
              local.get $3
              i32.ge_s
              br_if $block_1
              local.get $16
              local.get $3
              i32.const 2
              i32.add
              i32.add
              local.get $6
              i32.sub
              local.set $10
              local.get $6
              br $block_0
            end ;; $block_1
            local.get $5
            local.get $16
            local.get $17
            i32.sub
            local.get $6
            i32.sub
            i32.add
            local.set $10
            local.get $6
          end ;; $block_0
          local.set $7
          local.get $0
          i32.const 32
          local.get $2
          local.get $8
          local.get $10
          i32.add
          local.tee $3
          local.get $4
          call $_pad_524
          local.get $0
          local.get $9
          local.get $8
          call $_out
          local.get $0
          i32.const 48
          local.get $2
          local.get $3
          local.get $4
          i32.const 65536
          i32.xor
          call $_pad_524
          local.get $0
          local.get $12
          local.get $5
          local.get $17
          i32.sub
          local.tee $5
          call $_out
          local.get $0
          i32.const 48
          local.get $10
          local.get $5
          local.get $16
          local.get $7
          i32.sub
          local.tee $7
          i32.add
          i32.sub
          i32.const 0
          i32.const 0
          call $_pad_524
          local.get $0
          local.get $6
          local.get $7
          call $_out
          local.get $0
          i32.const 32
          local.get $2
          local.get $3
          local.get $4
          i32.const 8192
          i32.xor
          call $_pad_524
          local.get $3
          br $block
        end ;; $if_2
        local.get $6
        if $if_11
          local.get $8
          local.get $8
          i32.load
          i32.const -28
          i32.add
          local.tee $6
          i32.store
          local.get $1
          f64.const 0x1.0000000000000p+28
          f64.mul
          local.set $1
        else
          local.get $8
          i32.load
          local.set $6
        end ;; $if_11
        i32.const 6
        local.get $3
        local.get $3
        i32.const 0
        i32.lt_s
        select
        local.set $14
        local.get $7
        local.get $7
        i32.const 288
        i32.add
        local.get $6
        i32.const 0
        i32.lt_s
        select
        local.tee $10
        local.set $7
        loop $loop_1
          local.get $7
          local.get $1
          local.get $1
          f64.ne
          if $if_12 (result i32)
            i32.const 0
          else
            local.get $1
            f64.const 0x1.0000000000000p+32
            f64.ge
            if $if_13 (result i32)
              i32.const 0
            else
              local.get $1
              f64.const -0x1.0000000000000p-0
              f64.le
              if $if_14 (result i32)
                i32.const 0
              else
                local.get $1
                i32.trunc_f64_u
              end ;; $if_14
            end ;; $if_13
          end ;; $if_12
          local.tee $3
          i32.store
          local.get $7
          i32.const 4
          i32.add
          local.set $7
          local.get $1
          local.get $3
          f64.convert_i32_u
          f64.sub
          f64.const 0x1.dcd6500000000p+29
          f64.mul
          local.tee $1
          f64.const 0x0.0000000000000p-1023
          f64.ne
          br_if $loop_1
        end ;; $loop_1
        local.get $6
        i32.const 0
        i32.gt_s
        if $if_15
          local.get $10
          local.set $3
          loop $loop_2
            local.get $6
            i32.const 29
            local.get $6
            i32.const 29
            i32.lt_s
            select
            local.set $11
            local.get $7
            i32.const -4
            i32.add
            local.tee $6
            local.get $3
            i32.ge_u
            if $if_16
              local.get $11
              i64.extend_i32_u
              local.set $26
              i32.const 0
              local.set $9
              loop $loop_3
                local.get $9
                i64.extend_i32_u
                local.get $6
                i32.load
                i64.extend_i32_u
                local.get $26
                i64.shl
                i64.add
                local.tee $27
                i64.const 1000000000
                i64.div_u
                local.set $25
                local.get $6
                local.get $27
                local.get $25
                i64.const 1000000000
                i64.mul
                i64.sub
                i64.store32
                local.get $25
                i32.wrap_i64
                local.set $9
                local.get $6
                i32.const -4
                i32.add
                local.tee $6
                local.get $3
                i32.ge_u
                br_if $loop_3
              end ;; $loop_3
              local.get $9
              if $if_17
                local.get $3
                i32.const -4
                i32.add
                local.tee $3
                local.get $9
                i32.store
              end ;; $if_17
            end ;; $if_16
            local.get $7
            local.get $3
            i32.gt_u
            if $if_18
              block $block_2
                loop $loop_4 (result i32)
                  local.get $7
                  i32.const -4
                  i32.add
                  local.tee $6
                  i32.load
                  br_if $block_2
                  local.get $6
                  local.get $3
                  i32.gt_u
                  if $if_19 (result i32)
                    local.get $6
                    local.set $7
                    br $loop_4
                  else
                    local.get $6
                  end ;; $if_19
                end ;; $loop_4
                local.set $7
              end ;; $block_2
            end ;; $if_18
            local.get $8
            local.get $8
            i32.load
            local.get $11
            i32.sub
            local.tee $6
            i32.store
            local.get $6
            i32.const 0
            i32.gt_s
            br_if $loop_2
          end ;; $loop_2
        else
          local.get $10
          local.set $3
        end ;; $if_15
        local.get $6
        i32.const 0
        i32.lt_s
        if $if_20
          local.get $14
          i32.const 25
          i32.add
          i32.const 9
          i32.div_s
          i32.const 1
          i32.add
          local.set $15
          local.get $13
          i32.const 102
          i32.eq
          local.set $20
          local.get $6
          local.set $9
          local.get $3
          local.set $6
          local.get $7
          local.set $3
          loop $loop_5 (result i32)
            i32.const 0
            local.get $9
            i32.sub
            local.tee $7
            i32.const 9
            local.get $7
            i32.const 9
            i32.lt_s
            select
            local.set $11
            local.get $6
            local.get $3
            i32.lt_u
            if $if_21
              i32.const 1
              local.get $11
              i32.shl
              i32.const -1
              i32.add
              local.set $22
              i32.const 1000000000
              local.get $11
              i32.shr_u
              local.set $23
              i32.const 0
              local.set $9
              local.get $6
              local.set $7
              loop $loop_6
                local.get $7
                local.get $9
                local.get $7
                i32.load
                local.tee $24
                local.get $11
                i32.shr_u
                i32.add
                i32.store
                local.get $22
                local.get $24
                i32.and
                local.get $23
                i32.mul
                local.set $9
                local.get $7
                i32.const 4
                i32.add
                local.tee $7
                local.get $3
                i32.lt_u
                br_if $loop_6
              end ;; $loop_6
              local.get $6
              local.get $6
              i32.const 4
              i32.add
              local.get $6
              i32.load
              select
              local.set $7
              local.get $9
              if $if_22
                local.get $3
                local.get $9
                i32.store
                local.get $3
                i32.const 4
                i32.add
                local.set $3
              end ;; $if_22
            else
              local.get $6
              local.get $6
              i32.const 4
              i32.add
              local.get $6
              i32.load
              select
              local.set $7
            end ;; $if_21
            local.get $10
            local.get $7
            local.get $20
            select
            local.tee $6
            local.get $15
            i32.const 2
            i32.shl
            i32.add
            local.get $3
            local.get $3
            local.get $6
            i32.sub
            i32.const 2
            i32.shr_s
            local.get $15
            i32.gt_s
            select
            local.set $3
            local.get $8
            local.get $8
            i32.load
            local.get $11
            i32.add
            local.tee $9
            i32.store
            local.get $9
            i32.const 0
            i32.lt_s
            if $if_23 (result i32)
              local.get $7
              local.set $6
              br $loop_5
            else
              local.get $3
              local.set $9
              local.get $7
            end ;; $if_23
          end ;; $loop_5
          local.set $3
        else
          local.get $7
          local.set $9
        end ;; $if_20
        local.get $10
        local.set $11
        local.get $3
        local.get $9
        i32.lt_u
        if $if_24
          local.get $11
          local.get $3
          i32.sub
          i32.const 2
          i32.shr_s
          i32.const 9
          i32.mul
          local.set $7
          local.get $3
          i32.load
          local.tee $10
          i32.const 10
          i32.ge_u
          if $if_25
            i32.const 10
            local.set $6
            loop $loop_7
              local.get $7
              i32.const 1
              i32.add
              local.set $7
              local.get $10
              local.get $6
              i32.const 10
              i32.mul
              local.tee $6
              i32.ge_u
              br_if $loop_7
            end ;; $loop_7
          end ;; $if_25
        else
          i32.const 0
          local.set $7
        end ;; $if_24
        local.get $14
        i32.const 0
        local.get $7
        local.get $13
        i32.const 102
        i32.eq
        select
        i32.sub
        local.get $13
        i32.const 103
        i32.eq
        local.tee $20
        local.get $14
        i32.const 0
        i32.ne
        local.tee $22
        i32.and
        i32.const 31
        i32.shl
        i32.const 31
        i32.shr_s
        i32.add
        local.tee $6
        local.get $9
        local.get $11
        i32.sub
        i32.const 2
        i32.shr_s
        i32.const 9
        i32.mul
        i32.const -9
        i32.add
        i32.lt_s
        if $if_26 (result i32)
          local.get $6
          i32.const 9216
          i32.add
          local.tee $6
          i32.const 9
          i32.div_s
          local.set $13
          local.get $6
          local.get $13
          i32.const 9
          i32.mul
          i32.sub
          local.tee $6
          i32.const 8
          i32.lt_s
          if $if_27
            i32.const 10
            local.set $10
            loop $loop_8
              local.get $6
              i32.const 1
              i32.add
              local.set $8
              local.get $10
              i32.const 10
              i32.mul
              local.set $10
              local.get $6
              i32.const 7
              i32.lt_s
              if $if_28
                local.get $8
                local.set $6
                br $loop_8
              end ;; $if_28
            end ;; $loop_8
          else
            i32.const 10
            local.set $10
          end ;; $if_27
          local.get $11
          local.get $13
          i32.const 2
          i32.shl
          i32.add
          i32.const -4092
          i32.add
          local.tee $6
          i32.load
          local.tee $13
          local.set $8
          local.get $10
          if $if_29 (result i32)
            local.get $8
            local.get $10
            i32.div_u
          else
            i32.const 0
          end ;; $if_29
          local.set $15
          local.get $6
          i32.const 4
          i32.add
          local.get $9
          i32.eq
          local.tee $23
          local.get $13
          local.get $10
          local.get $15
          i32.mul
          i32.sub
          local.tee $8
          i32.eqz
          i32.and
          i32.eqz
          if $if_30
            f64.const 0x1.0000000000001p+53
            f64.const 0x1.0000000000000p+53
            local.get $15
            i32.const 1
            i32.and
            select
            local.set $1
            f64.const 0x1.0000000000000p-1
            f64.const 0x1.0000000000000p-0
            f64.const 0x1.8000000000000p-0
            local.get $23
            local.get $8
            local.get $10
            i32.const 1
            i32.shr_u
            local.tee $15
            i32.eq
            i32.and
            select
            local.get $8
            local.get $15
            i32.lt_u
            select
            local.set $28
            local.get $19
            if $if_31
              local.get $1
              f64.neg
              local.get $1
              local.get $18
              i32.load8_s
              i32.const 45
              i32.eq
              local.tee $15
              select
              local.set $1
              local.get $28
              f64.neg
              local.get $28
              local.get $15
              select
              local.set $28
            end ;; $if_31
            local.get $6
            local.get $13
            local.get $8
            i32.sub
            local.tee $8
            i32.store
            local.get $1
            local.get $28
            f64.add
            local.get $1
            f64.ne
            if $if_32
              local.get $6
              local.get $8
              local.get $10
              i32.add
              local.tee $7
              i32.store
              local.get $7
              i32.const 999999999
              i32.gt_u
              if $if_33
                loop $loop_9
                  local.get $6
                  i32.const 0
                  i32.store
                  local.get $6
                  i32.const -4
                  i32.add
                  local.tee $6
                  local.get $3
                  i32.lt_u
                  if $if_34
                    local.get $3
                    i32.const -4
                    i32.add
                    local.tee $3
                    i32.const 0
                    i32.store
                  end ;; $if_34
                  local.get $6
                  local.get $6
                  i32.load
                  i32.const 1
                  i32.add
                  local.tee $7
                  i32.store
                  local.get $7
                  i32.const 999999999
                  i32.gt_u
                  br_if $loop_9
                end ;; $loop_9
              end ;; $if_33
              local.get $11
              local.get $3
              i32.sub
              i32.const 2
              i32.shr_s
              i32.const 9
              i32.mul
              local.set $7
              local.get $3
              i32.load
              local.tee $8
              i32.const 10
              i32.ge_u
              if $if_35
                i32.const 10
                local.set $10
                loop $loop_10
                  local.get $7
                  i32.const 1
                  i32.add
                  local.set $7
                  local.get $8
                  local.get $10
                  i32.const 10
                  i32.mul
                  local.tee $10
                  i32.ge_u
                  br_if $loop_10
                end ;; $loop_10
              end ;; $if_35
            end ;; $if_32
          end ;; $if_30
          local.get $3
          local.set $10
          local.get $7
          local.set $8
          local.get $6
          i32.const 4
          i32.add
          local.tee $3
          local.get $9
          local.get $9
          local.get $3
          i32.gt_u
          select
        else
          local.get $3
          local.set $10
          local.get $7
          local.set $8
          local.get $9
        end ;; $if_26
        local.tee $3
        local.get $10
        i32.gt_u
        if $if_36 (result i32)
          loop $loop_11 (result i32)
            block $block_3 (result i32)
              local.get $3
              i32.const -4
              i32.add
              local.tee $7
              i32.load
              if $if_37
                local.get $3
                local.set $7
                i32.const 1
                br $block_3
              end ;; $if_37
              local.get $7
              local.get $10
              i32.gt_u
              if $if_38 (result i32)
                local.get $7
                local.set $3
                br $loop_11
              else
                i32.const 0
              end ;; $if_38
            end ;; $block_3
          end ;; $loop_11
        else
          local.get $3
          local.set $7
          i32.const 0
        end ;; $if_36
        local.set $13
        local.get $20
        if $if_39 (result i32)
          local.get $22
          i32.const 1
          i32.xor
          local.get $14
          i32.add
          local.tee $3
          local.get $8
          i32.gt_s
          local.get $8
          i32.const -5
          i32.gt_s
          i32.and
          if $if_40 (result i32)
            local.get $3
            i32.const -1
            i32.add
            local.get $8
            i32.sub
            local.set $9
            local.get $5
            i32.const -1
            i32.add
          else
            local.get $3
            i32.const -1
            i32.add
            local.set $9
            local.get $5
            i32.const -2
            i32.add
          end ;; $if_40
          local.set $5
          local.get $4
          i32.const 8
          i32.and
          if $if_41 (result i32)
            local.get $9
          else
            local.get $13
            if $if_42
              local.get $7
              i32.const -4
              i32.add
              i32.load
              local.tee $14
              if $if_43
                local.get $14
                i32.const 10
                i32.rem_u
                if $if_44
                  i32.const 0
                  local.set $3
                else
                  i32.const 10
                  local.set $6
                  i32.const 0
                  local.set $3
                  loop $loop_12
                    local.get $3
                    i32.const 1
                    i32.add
                    local.set $3
                    local.get $6
                    i32.const 10
                    i32.mul
                    local.tee $6
                    local.tee $15
                    if $if_45 (result i32)
                      local.get $14
                      local.get $15
                      i32.rem_u
                    else
                      i32.const 0
                    end ;; $if_45
                    i32.eqz
                    br_if $loop_12
                  end ;; $loop_12
                end ;; $if_44
              else
                i32.const 9
                local.set $3
              end ;; $if_43
            else
              i32.const 9
              local.set $3
            end ;; $if_42
            local.get $7
            local.get $11
            i32.sub
            i32.const 2
            i32.shr_s
            i32.const 9
            i32.mul
            i32.const -9
            i32.add
            local.set $6
            local.get $5
            i32.const 32
            i32.or
            i32.const 102
            i32.eq
            if $if_46 (result i32)
              local.get $9
              local.get $6
              local.get $3
              i32.sub
              local.tee $3
              i32.const 0
              local.get $3
              i32.const 0
              i32.gt_s
              select
              local.tee $3
              local.get $9
              local.get $3
              i32.lt_s
              select
            else
              local.get $9
              local.get $6
              local.get $8
              i32.add
              local.get $3
              i32.sub
              local.tee $3
              i32.const 0
              local.get $3
              i32.const 0
              i32.gt_s
              select
              local.tee $3
              local.get $9
              local.get $3
              i32.lt_s
              select
            end ;; $if_46
          end ;; $if_41
        else
          local.get $14
        end ;; $if_39
        local.set $3
        i32.const 0
        local.get $8
        i32.sub
        local.set $6
        local.get $0
        i32.const 32
        local.get $2
        local.get $5
        i32.const 32
        i32.or
        i32.const 102
        i32.eq
        local.tee $15
        if $if_47 (result i32)
          i32.const 0
          local.set $9
          local.get $8
          i32.const 0
          local.get $8
          i32.const 0
          i32.gt_s
          select
        else
          local.get $16
          local.get $6
          local.get $8
          local.get $8
          i32.const 0
          i32.lt_s
          select
          i64.extend_i32_s
          local.get $16
          call $_fmt_u
          local.tee $6
          i32.sub
          i32.const 2
          i32.lt_s
          if $if_48
            loop $loop_13
              local.get $6
              i32.const -1
              i32.add
              local.tee $6
              i32.const 48
              i32.store8
              local.get $16
              local.get $6
              i32.sub
              i32.const 2
              i32.lt_s
              br_if $loop_13
            end ;; $loop_13
          end ;; $if_48
          local.get $6
          i32.const -1
          i32.add
          local.get $8
          i32.const 31
          i32.shr_s
          i32.const 2
          i32.and
          i32.const 43
          i32.add
          i32.store8
          local.get $6
          i32.const -2
          i32.add
          local.tee $9
          local.get $5
          i32.store8
          local.get $16
          local.get $9
          i32.sub
        end ;; $if_47
        local.get $19
        i32.const 1
        i32.add
        local.get $3
        i32.add
        i32.const 1
        local.get $4
        i32.const 3
        i32.shr_u
        i32.const 1
        i32.and
        local.get $3
        i32.const 0
        i32.ne
        local.tee $20
        select
        i32.add
        i32.add
        local.tee $14
        local.get $4
        call $_pad_524
        local.get $0
        local.get $18
        local.get $19
        call $_out
        local.get $0
        i32.const 48
        local.get $2
        local.get $14
        local.get $4
        i32.const 65536
        i32.xor
        call $_pad_524
        local.get $15
        if $if_49
          local.get $12
          i32.const 9
          i32.add
          local.tee $8
          local.set $16
          local.get $12
          i32.const 8
          i32.add
          local.set $9
          local.get $11
          local.get $10
          local.get $10
          local.get $11
          i32.gt_u
          select
          local.tee $10
          local.set $6
          loop $loop_14
            local.get $6
            i32.load
            i64.extend_i32_u
            local.get $8
            call $_fmt_u
            local.set $5
            local.get $6
            local.get $10
            i32.eq
            if $if_50
              local.get $5
              local.get $8
              i32.eq
              if $if_51
                local.get $9
                i32.const 48
                i32.store8
                local.get $9
                local.set $5
              end ;; $if_51
            else
              local.get $5
              local.get $12
              i32.gt_u
              if $if_52
                local.get $12
                i32.const 48
                local.get $5
                local.get $17
                i32.sub
                call $_memset
                drop
                loop $loop_15
                  local.get $5
                  i32.const -1
                  i32.add
                  local.tee $5
                  local.get $12
                  i32.gt_u
                  br_if $loop_15
                end ;; $loop_15
              end ;; $if_52
            end ;; $if_50
            local.get $0
            local.get $5
            local.get $16
            local.get $5
            i32.sub
            call $_out
            local.get $6
            i32.const 4
            i32.add
            local.tee $5
            local.get $11
            i32.le_u
            if $if_53
              local.get $5
              local.set $6
              br $loop_14
            end ;; $if_53
          end ;; $loop_14
          local.get $4
          i32.const 8
          i32.and
          i32.eqz
          local.get $20
          i32.const 1
          i32.xor
          i32.and
          i32.eqz
          if $if_54
            local.get $0
            i32.const 2180
            i32.const 1
            call $_out
          end ;; $if_54
          local.get $0
          i32.const 48
          local.get $5
          local.get $7
          i32.lt_u
          local.get $3
          i32.const 0
          i32.gt_s
          i32.and
          if $if_55 (result i32)
            loop $loop_16 (result i32)
              local.get $5
              i32.load
              i64.extend_i32_u
              local.get $8
              call $_fmt_u
              local.tee $6
              local.get $12
              i32.gt_u
              if $if_56
                local.get $12
                i32.const 48
                local.get $6
                local.get $17
                i32.sub
                call $_memset
                drop
                loop $loop_17
                  local.get $6
                  i32.const -1
                  i32.add
                  local.tee $6
                  local.get $12
                  i32.gt_u
                  br_if $loop_17
                end ;; $loop_17
              end ;; $if_56
              local.get $0
              local.get $6
              local.get $3
              i32.const 9
              local.get $3
              i32.const 9
              i32.lt_s
              select
              call $_out
              local.get $3
              i32.const -9
              i32.add
              local.set $6
              local.get $5
              i32.const 4
              i32.add
              local.tee $5
              local.get $7
              i32.lt_u
              local.get $3
              i32.const 9
              i32.gt_s
              i32.and
              if $if_57 (result i32)
                local.get $6
                local.set $3
                br $loop_16
              else
                local.get $6
              end ;; $if_57
            end ;; $loop_16
          else
            local.get $3
          end ;; $if_55
          i32.const 9
          i32.add
          i32.const 9
          i32.const 0
          call $_pad_524
        else
          local.get $0
          i32.const 48
          local.get $10
          local.get $7
          local.get $10
          i32.const 4
          i32.add
          local.get $13
          select
          local.tee $18
          i32.lt_u
          local.get $3
          i32.const -1
          i32.gt_s
          i32.and
          if $if_58 (result i32)
            local.get $4
            i32.const 8
            i32.and
            i32.eqz
            local.set $19
            local.get $12
            i32.const 9
            i32.add
            local.tee $11
            local.set $13
            i32.const 0
            local.get $17
            i32.sub
            local.set $17
            local.get $12
            i32.const 8
            i32.add
            local.set $8
            local.get $10
            local.set $7
            local.get $3
            local.set $5
            loop $loop_18 (result i32)
              local.get $11
              local.get $7
              i32.load
              i64.extend_i32_u
              local.get $11
              call $_fmt_u
              local.tee $3
              i32.eq
              if $if_59
                local.get $8
                i32.const 48
                i32.store8
                local.get $8
                local.set $3
              end ;; $if_59
              block $block_4
                local.get $7
                local.get $10
                i32.eq
                if $if_60
                  local.get $3
                  i32.const 1
                  i32.add
                  local.set $6
                  local.get $0
                  local.get $3
                  i32.const 1
                  call $_out
                  local.get $5
                  i32.const 1
                  i32.lt_s
                  local.get $19
                  i32.and
                  if $if_61
                    local.get $6
                    local.set $3
                    br $block_4
                  end ;; $if_61
                  local.get $0
                  i32.const 2180
                  i32.const 1
                  call $_out
                  local.get $6
                  local.set $3
                else
                  local.get $3
                  local.get $12
                  i32.le_u
                  br_if $block_4
                  local.get $12
                  i32.const 48
                  local.get $3
                  local.get $17
                  i32.add
                  call $_memset
                  drop
                  loop $loop_19
                    local.get $3
                    i32.const -1
                    i32.add
                    local.tee $3
                    local.get $12
                    i32.gt_u
                    br_if $loop_19
                  end ;; $loop_19
                end ;; $if_60
              end ;; $block_4
              local.get $0
              local.get $3
              local.get $13
              local.get $3
              i32.sub
              local.tee $3
              local.get $5
              local.get $5
              local.get $3
              i32.gt_s
              select
              call $_out
              local.get $7
              i32.const 4
              i32.add
              local.tee $7
              local.get $18
              i32.lt_u
              local.get $5
              local.get $3
              i32.sub
              local.tee $5
              i32.const -1
              i32.gt_s
              i32.and
              br_if $loop_18
              local.get $5
            end ;; $loop_18
          else
            local.get $3
          end ;; $if_58
          i32.const 18
          i32.add
          i32.const 18
          i32.const 0
          call $_pad_524
          local.get $0
          local.get $9
          local.get $16
          local.get $9
          i32.sub
          call $_out
        end ;; $if_49
        local.get $0
        i32.const 32
        local.get $2
        local.get $14
        local.get $4
        i32.const 8192
        i32.xor
        call $_pad_524
        local.get $14
      end ;; $block
    end ;; $if_0
    local.set $0
    local.get $21
    global.set $22
    local.get $2
    local.get $0
    local.get $0
    local.get $2
    i32.lt_s
    select
    )
  
  (func $_pop_arg_long_double (type $2)
    (param $0 i32)
    (param $1 i32)
    (local $2 i32)
    (local $3 f64)
    local.get $1
    i32.load
    i32.const 7
    i32.add
    i32.const -8
    i32.and
    local.tee $2
    f64.load
    local.set $3
    local.get $1
    local.get $2
    i32.const 8
    i32.add
    i32.store
    local.get $0
    local.get $3
    f64.store
    )
  
  (func $___vfprintf_internal (type $12)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    global.get $22
    local.set $2
    global.get $22
    i32.const 224
    i32.add
    global.set $22
    local.get $2
    i32.const 160
    i32.add
    local.tee $3
    i64.const 0
    i64.store
    local.get $3
    i64.const 0
    i64.store offset=8
    local.get $3
    i64.const 0
    i64.store offset=16
    local.get $3
    i64.const 0
    i64.store offset=24
    local.get $3
    i64.const 0
    i64.store offset=32
    local.get $2
    i32.const 208
    i32.add
    local.tee $4
    local.get $1
    i32.load
    i32.store
    i32.const 0
    local.get $4
    local.get $2
    i32.const 80
    i32.add
    local.tee $1
    local.get $3
    call $_printf_core
    i32.const 0
    i32.lt_s
    if $if (result i32)
      i32.const -1
    else
      local.get $0
      i32.load offset=76
      i32.const -1
      i32.gt_s
      if $if_0 (result i32)
        i32.const 1
      else
        i32.const 0
      end ;; $if_0
      drop
      local.get $0
      i32.load
      local.set $5
      local.get $0
      i32.load8_s offset=74
      i32.const 1
      i32.lt_s
      if $if_1
        local.get $0
        local.get $5
        i32.const -33
        i32.and
        i32.store
      end ;; $if_1
      local.get $0
      i32.load offset=48
      if $if_2
        local.get $0
        local.get $4
        local.get $1
        local.get $3
        call $_printf_core
        local.set $1
      else
        local.get $0
        i32.load offset=44
        local.set $6
        local.get $0
        local.get $2
        i32.store offset=44
        local.get $0
        local.get $2
        i32.store offset=28
        local.get $0
        local.get $2
        i32.store offset=20
        local.get $0
        i32.const 80
        i32.store offset=48
        local.get $0
        local.get $2
        i32.const 80
        i32.add
        i32.store offset=16
        local.get $0
        local.get $4
        local.get $1
        local.get $3
        call $_printf_core
        local.set $1
        local.get $6
        if $if_3
          local.get $0
          i32.const 0
          i32.const 0
          local.get $0
          i32.load offset=36
          i32.const 3
          i32.and
          i32.const 2
          i32.add
          call_indirect $18 (type $0)
          drop
          local.get $1
          i32.const -1
          local.get $0
          i32.load offset=20
          select
          local.set $1
          local.get $0
          local.get $6
          i32.store offset=44
          local.get $0
          i32.const 0
          i32.store offset=48
          local.get $0
          i32.const 0
          i32.store offset=16
          local.get $0
          i32.const 0
          i32.store offset=28
          local.get $0
          i32.const 0
          i32.store offset=20
        end ;; $if_3
      end ;; $if_2
      local.get $0
      local.get $0
      i32.load
      local.tee $0
      local.get $5
      i32.const 32
      i32.and
      i32.or
      i32.store
      i32.const -1
      local.get $1
      local.get $0
      i32.const 32
      i32.and
      select
    end ;; $if
    local.set $0
    local.get $2
    global.set $22
    local.get $0
    )
  
  (func $_printf_core (type $13)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (result i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    (local $7 i32)
    (local $8 i32)
    (local $9 i32)
    (local $10 i32)
    (local $11 i32)
    (local $12 i32)
    (local $13 i32)
    (local $14 i32)
    (local $15 i32)
    (local $16 i32)
    (local $17 i32)
    (local $18 i32)
    (local $19 i32)
    (local $20 i32)
    (local $21 i32)
    (local $22 i32)
    (local $23 i32)
    (local $24 i32)
    (local $25 i64)
    global.get $22
    local.set $15
    global.get $22
    i32.const -64
    i32.sub
    global.set $22
    local.get $15
    i32.const 40
    i32.add
    local.set $10
    local.get $15
    i32.const 48
    i32.add
    local.set $24
    local.get $15
    i32.const 60
    i32.add
    local.set $20
    local.get $15
    i32.const 56
    i32.add
    local.tee $11
    i32.const 2182
    i32.store
    local.get $0
    i32.const 0
    i32.ne
    local.set $17
    local.get $15
    i32.const 40
    i32.add
    local.tee $19
    local.set $18
    local.get $15
    i32.const 39
    i32.add
    local.set $21
    block $block
      block $block_0
        loop $loop
          block $block_1
            loop $loop_0
              local.get $9
              i32.const -1
              i32.gt_s
              if $if
                local.get $4
                i32.const 2147483647
                local.get $9
                i32.sub
                i32.gt_s
                if $if_0 (result i32)
                  i32.const 2496
                  i32.const 75
                  i32.store
                  i32.const -1
                else
                  local.get $4
                  local.get $9
                  i32.add
                end ;; $if_0
                local.set $9
              end ;; $if
              local.get $11
              i32.load
              local.tee $12
              i32.load8_s
              local.tee $5
              i32.eqz
              br_if $block_0
              local.get $12
              local.set $4
              block $block_2
                block $block_3
                  loop $loop_1
                    block $block_4
                      block $block_5
                        local.get $5
                        i32.const 24
                        i32.shl
                        i32.const 24
                        i32.shr_s
                        br_table
                          $block_4 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5
                          $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5 $block_5
                          $block_5 $block_5 $block_5 $block_5 $block_5 $block_3
                          $block_5 ;; default
                      end ;; $block_5
                      local.get $11
                      local.get $4
                      i32.const 1
                      i32.add
                      local.tee $4
                      i32.store
                      local.get $4
                      i32.load8_s
                      local.set $5
                      br $loop_1
                    end ;; $block_4
                  end ;; $loop_1
                  br $block_2
                end ;; $block_3
                local.get $4
                local.set $5
                loop $loop_2 (result i32)
                  local.get $4
                  i32.load8_s offset=1
                  i32.const 37
                  i32.ne
                  if $if_1
                    local.get $5
                    local.set $4
                    br $block_2
                  end ;; $if_1
                  local.get $5
                  i32.const 1
                  i32.add
                  local.set $5
                  local.get $11
                  local.get $4
                  i32.const 2
                  i32.add
                  local.tee $4
                  i32.store
                  local.get $4
                  i32.load8_s
                  i32.const 37
                  i32.eq
                  br_if $loop_2
                  local.get $5
                end ;; $loop_2
                local.set $4
              end ;; $block_2
              local.get $4
              local.get $12
              i32.sub
              local.set $4
              local.get $17
              if $if_2
                local.get $0
                local.get $12
                local.get $4
                call $_out
              end ;; $if_2
              local.get $4
              br_if $loop_0
            end ;; $loop_0
            local.get $11
            local.get $11
            i32.load
            local.tee $4
            local.get $11
            i32.load
            i32.load8_s offset=1
            i32.const -48
            i32.add
            i32.const 10
            i32.ge_u
            if $if_3 (result i32)
              i32.const -1
              local.set $14
              i32.const 1
            else
              local.get $4
              i32.load8_s offset=2
              i32.const 36
              i32.eq
              if $if_4 (result i32)
                local.get $4
                i32.load8_s offset=1
                i32.const -48
                i32.add
                local.set $14
                i32.const 1
                local.set $7
                i32.const 3
              else
                i32.const -1
                local.set $14
                i32.const 1
              end ;; $if_4
            end ;; $if_3
            i32.add
            local.tee $4
            i32.store
            local.get $4
            i32.load8_s
            local.tee $6
            i32.const -32
            i32.add
            local.tee $5
            i32.const 31
            i32.gt_u
            i32.const 1
            local.get $5
            i32.shl
            i32.const 75913
            i32.and
            i32.eqz
            i32.or
            if $if_5
              i32.const 0
              local.set $5
            else
              i32.const 0
              local.set $6
              loop $loop_3
                local.get $6
                i32.const 1
                local.get $5
                i32.shl
                i32.or
                local.set $5
                local.get $11
                local.get $4
                i32.const 1
                i32.add
                local.tee $4
                i32.store
                local.get $4
                i32.load8_s
                local.tee $6
                i32.const -32
                i32.add
                local.tee $8
                i32.const 31
                i32.gt_u
                i32.const 1
                local.get $8
                i32.shl
                i32.const 75913
                i32.and
                i32.eqz
                i32.or
                i32.eqz
                if $if_6
                  local.get $5
                  local.set $6
                  local.get $8
                  local.set $5
                  br $loop_3
                end ;; $if_6
              end ;; $loop_3
            end ;; $if_5
            local.get $6
            i32.const 255
            i32.and
            i32.const 42
            i32.eq
            if $if_7 (result i32)
              block $block_6 (result i32)
                block $block_7
                  local.get $4
                  i32.load8_s offset=1
                  i32.const -48
                  i32.add
                  i32.const 10
                  i32.ge_u
                  br_if $block_7
                  local.get $11
                  i32.load
                  local.tee $4
                  i32.load8_s offset=2
                  i32.const 36
                  i32.ne
                  br_if $block_7
                  local.get $4
                  i32.load8_s offset=1
                  i32.const -48
                  i32.add
                  i32.const 2
                  i32.shl
                  local.get $3
                  i32.add
                  i32.const 10
                  i32.store
                  i32.const 1
                  local.set $8
                  local.get $4
                  i32.const 3
                  i32.add
                  local.set $6
                  local.get $4
                  i32.load8_s offset=1
                  i32.const -48
                  i32.add
                  i32.const 3
                  i32.shl
                  local.get $2
                  i32.add
                  i64.load
                  i32.wrap_i64
                  br $block_6
                end ;; $block_7
                local.get $7
                if $if_8
                  i32.const -1
                  local.set $9
                  br $block_1
                end ;; $if_8
                local.get $17
                if $if_9
                  local.get $1
                  i32.load
                  i32.const 3
                  i32.add
                  i32.const -4
                  i32.and
                  local.tee $6
                  i32.load
                  local.set $4
                  local.get $1
                  local.get $6
                  i32.const 4
                  i32.add
                  i32.store
                else
                  i32.const 0
                  local.set $4
                end ;; $if_9
                i32.const 0
                local.set $8
                local.get $11
                i32.load
                i32.const 1
                i32.add
                local.set $6
                local.get $4
              end ;; $block_6
              local.set $7
              local.get $11
              local.get $6
              i32.store
              local.get $6
              local.set $4
              local.get $5
              i32.const 8192
              i32.or
              local.get $5
              local.get $7
              i32.const 0
              i32.lt_s
              local.tee $5
              select
              local.set $13
              i32.const 0
              local.get $7
              i32.sub
              local.get $7
              local.get $5
              select
              local.set $16
              local.get $8
            else
              local.get $11
              call $_getint
              local.tee $16
              i32.const 0
              i32.lt_s
              if $if_10
                i32.const -1
                local.set $9
                br $block_1
              end ;; $if_10
              local.get $11
              i32.load
              local.set $4
              local.get $5
              local.set $13
              local.get $7
            end ;; $if_7
            local.set $22
            local.get $4
            i32.load8_s
            i32.const 46
            i32.eq
            if $if_11
              block $block_8
                local.get $4
                i32.const 1
                i32.add
                local.set $5
                local.get $4
                i32.load8_s offset=1
                i32.const 42
                i32.ne
                if $if_12
                  local.get $11
                  local.get $5
                  i32.store
                  local.get $11
                  call $_getint
                  local.set $4
                  local.get $11
                  i32.load
                  local.set $7
                  br $block_8
                end ;; $if_12
                local.get $4
                i32.load8_s offset=2
                i32.const -48
                i32.add
                i32.const 10
                i32.lt_u
                if $if_13
                  local.get $11
                  i32.load
                  local.tee $5
                  i32.load8_s offset=3
                  i32.const 36
                  i32.eq
                  if $if_14
                    local.get $5
                    i32.load8_s offset=2
                    i32.const -48
                    i32.add
                    i32.const 2
                    i32.shl
                    local.get $3
                    i32.add
                    i32.const 10
                    i32.store
                    local.get $5
                    i32.load8_s offset=2
                    i32.const -48
                    i32.add
                    i32.const 3
                    i32.shl
                    local.get $2
                    i32.add
                    i64.load
                    i32.wrap_i64
                    local.set $4
                    local.get $11
                    local.get $5
                    i32.const 4
                    i32.add
                    local.tee $7
                    i32.store
                    br $block_8
                  end ;; $if_14
                end ;; $if_13
                local.get $22
                if $if_15
                  i32.const -1
                  local.set $9
                  br $block_1
                end ;; $if_15
                local.get $17
                if $if_16
                  local.get $1
                  i32.load
                  i32.const 3
                  i32.add
                  i32.const -4
                  i32.and
                  local.tee $5
                  i32.load
                  local.set $4
                  local.get $1
                  local.get $5
                  i32.const 4
                  i32.add
                  i32.store
                else
                  i32.const 0
                  local.set $4
                end ;; $if_16
                local.get $11
                local.get $11
                i32.load
                i32.const 2
                i32.add
                local.tee $7
                i32.store
              end ;; $block_8
            else
              local.get $4
              local.set $7
              i32.const -1
              local.set $4
            end ;; $if_11
            i32.const 0
            local.set $5
            loop $loop_4
              local.get $7
              i32.load8_s
              i32.const -65
              i32.add
              i32.const 57
              i32.gt_u
              if $if_17
                i32.const -1
                local.set $9
                br $block_1
              end ;; $if_17
              local.get $11
              local.get $7
              i32.const 1
              i32.add
              local.tee $6
              i32.store
              local.get $7
              i32.load8_s
              local.get $5
              i32.const 58
              i32.mul
              i32.add
              i32.load8_s offset=959
              local.tee $7
              i32.const 255
              i32.and
              local.tee $8
              i32.const -1
              i32.add
              i32.const 8
              i32.lt_u
              if $if_18
                local.get $6
                local.set $7
                local.get $8
                local.set $5
                br $loop_4
              end ;; $if_18
            end ;; $loop_4
            local.get $7
            i32.eqz
            if $if_19
              i32.const -1
              local.set $9
              br $block_1
            end ;; $if_19
            local.get $14
            i32.const -1
            i32.gt_s
            local.set $23
            block $block_9
              block $block_10
                local.get $7
                i32.const 19
                i32.eq
                if $if_20
                  local.get $23
                  if $if_21
                    i32.const -1
                    local.set $9
                    br $block_1
                  end ;; $if_21
                else
                  block $block_11
                    local.get $23
                    if $if_22
                      local.get $14
                      i32.const 2
                      i32.shl
                      local.get $3
                      i32.add
                      local.get $8
                      i32.store
                      local.get $10
                      local.get $14
                      i32.const 3
                      i32.shl
                      local.get $2
                      i32.add
                      i64.load
                      i64.store
                      br $block_11
                    end ;; $if_22
                    local.get $17
                    i32.eqz
                    if $if_23
                      i32.const 0
                      local.set $9
                      br $block_1
                    end ;; $if_23
                    local.get $10
                    local.get $8
                    local.get $1
                    call $_pop_arg
                    local.get $11
                    i32.load
                    local.set $6
                    br $block_10
                  end ;; $block_11
                end ;; $if_20
                local.get $17
                br_if $block_10
                i32.const 0
                local.set $4
                br $block_9
              end ;; $block_10
              local.get $13
              i32.const -65537
              i32.and
              local.tee $8
              local.get $13
              local.get $13
              i32.const 8192
              i32.and
              select
              local.set $7
              block $block_12
                block $block_13
                  block $block_14
                    block $block_15
                      block $block_16
                        block $block_17
                          block $block_18
                            block $block_19
                              block $block_20
                                block $block_21
                                  block $block_22
                                    block $block_23
                                      block $block_24
                                        block $block_25
                                          block $block_26
                                            block $block_27
                                              block $block_28
                                                local.get $6
                                                i32.const -1
                                                i32.add
                                                i32.load8_s
                                                local.tee $6
                                                i32.const -33
                                                i32.and
                                                local.get $6
                                                local.get $6
                                                i32.const 15
                                                i32.and
                                                i32.const 3
                                                i32.eq
                                                local.get $5
                                                i32.const 0
                                                i32.ne
                                                i32.and
                                                select
                                                local.tee $6
                                                i32.const 65
                                                i32.sub
                                                br_table
                                                  $block_19 $block_18 $block_21 $block_18 $block_19 $block_19 $block_19 $block_18 $block_18 $block_18 $block_18 $block_18 $block_18 $block_18 $block_18 $block_18
                                                  $block_18 $block_18 $block_20 $block_18 $block_18 $block_18 $block_18 $block_17 $block_18 $block_18 $block_18 $block_18 $block_18 $block_18 $block_18 $block_18
                                                  $block_19 $block_18 $block_23 $block_25 $block_19 $block_19 $block_19 $block_18 $block_25 $block_18 $block_18 $block_18 $block_18 $block_28 $block_26 $block_27
                                                  $block_18 $block_18 $block_22 $block_18 $block_24 $block_18 $block_18 $block_17
                                                  $block_18 ;; default
                                              end ;; $block_28
                                              block $block_29
                                                block $block_30
                                                  block $block_31
                                                    block $block_32
                                                      block $block_33
                                                        block $block_34
                                                          block $block_35
                                                            block $block_36
                                                              local.get $5
                                                              i32.const 255
                                                              i32.and
                                                              i32.const 24
                                                              i32.shl
                                                              i32.const 24
                                                              i32.shr_s
                                                              br_table
                                                                $block_36 $block_35 $block_34 $block_33 $block_32 $block_29 $block_31 $block_30
                                                                $block_29 ;; default
                                                            end ;; $block_36
                                                            local.get $10
                                                            i32.load
                                                            local.get $9
                                                            i32.store
                                                            i32.const 0
                                                            local.set $4
                                                            br $block_9
                                                          end ;; $block_35
                                                          local.get $10
                                                          i32.load
                                                          local.get $9
                                                          i32.store
                                                          i32.const 0
                                                          local.set $4
                                                          br $block_9
                                                        end ;; $block_34
                                                        local.get $10
                                                        i32.load
                                                        local.get $9
                                                        i64.extend_i32_s
                                                        i64.store
                                                        i32.const 0
                                                        local.set $4
                                                        br $block_9
                                                      end ;; $block_33
                                                      local.get $10
                                                      i32.load
                                                      local.get $9
                                                      i32.store16
                                                      i32.const 0
                                                      local.set $4
                                                      br $block_9
                                                    end ;; $block_32
                                                    local.get $10
                                                    i32.load
                                                    local.get $9
                                                    i32.store8
                                                    i32.const 0
                                                    local.set $4
                                                    br $block_9
                                                  end ;; $block_31
                                                  local.get $10
                                                  i32.load
                                                  local.get $9
                                                  i32.store
                                                  i32.const 0
                                                  local.set $4
                                                  br $block_9
                                                end ;; $block_30
                                                local.get $10
                                                i32.load
                                                local.get $9
                                                i64.extend_i32_s
                                                i64.store
                                                i32.const 0
                                                local.set $4
                                                br $block_9
                                              end ;; $block_29
                                              i32.const 0
                                              local.set $4
                                              br $block_9
                                            end ;; $block_27
                                            local.get $7
                                            i32.const 8
                                            i32.or
                                            local.set $7
                                            local.get $4
                                            i32.const 8
                                            local.get $4
                                            i32.const 8
                                            i32.gt_u
                                            select
                                            local.set $4
                                            i32.const 120
                                            local.set $6
                                            br $block_17
                                          end ;; $block_26
                                          local.get $4
                                          local.get $18
                                          local.get $10
                                          i64.load
                                          local.get $19
                                          call $_fmt_o
                                          local.tee $5
                                          i32.sub
                                          local.tee $6
                                          i32.const 1
                                          i32.add
                                          local.get $7
                                          i32.const 8
                                          i32.and
                                          i32.eqz
                                          local.get $4
                                          local.get $6
                                          i32.gt_s
                                          i32.or
                                          select
                                          local.set $4
                                          i32.const 0
                                          local.set $12
                                          i32.const 2128
                                          local.set $8
                                          br $block_14
                                        end ;; $block_25
                                        local.get $10
                                        i64.load
                                        local.tee $25
                                        i64.const 0
                                        i64.lt_s
                                        if $if_24 (result i32)
                                          local.get $10
                                          i64.const 0
                                          local.get $25
                                          i64.sub
                                          local.tee $25
                                          i64.store
                                          i32.const 2128
                                          local.set $8
                                          i32.const 1
                                        else
                                          i32.const 2129
                                          i32.const 2130
                                          i32.const 2128
                                          local.get $7
                                          i32.const 1
                                          i32.and
                                          select
                                          local.get $7
                                          i32.const 2048
                                          i32.and
                                          select
                                          local.set $8
                                          local.get $7
                                          i32.const 2049
                                          i32.and
                                          i32.const 0
                                          i32.ne
                                        end ;; $if_24
                                        local.set $12
                                        br $block_16
                                      end ;; $block_24
                                      local.get $10
                                      i64.load
                                      local.set $25
                                      i32.const 0
                                      local.set $12
                                      i32.const 2128
                                      local.set $8
                                      br $block_16
                                    end ;; $block_23
                                    local.get $21
                                    local.get $10
                                    i64.load
                                    i64.store8
                                    local.get $21
                                    local.set $6
                                    local.get $8
                                    local.set $7
                                    i32.const 1
                                    local.set $5
                                    i32.const 0
                                    local.set $12
                                    i32.const 2128
                                    local.set $8
                                    local.get $18
                                    local.set $4
                                    br $block_12
                                  end ;; $block_22
                                  local.get $10
                                  i32.load
                                  local.tee $5
                                  i32.const 2138
                                  local.get $5
                                  select
                                  local.tee $6
                                  local.get $4
                                  call $_memchr
                                  local.tee $13
                                  i32.eqz
                                  local.set $14
                                  local.get $8
                                  local.set $7
                                  local.get $4
                                  local.get $13
                                  local.get $6
                                  i32.sub
                                  local.get $14
                                  select
                                  local.set $5
                                  i32.const 0
                                  local.set $12
                                  i32.const 2128
                                  local.set $8
                                  local.get $4
                                  local.get $6
                                  i32.add
                                  local.get $13
                                  local.get $14
                                  select
                                  local.set $4
                                  br $block_12
                                end ;; $block_21
                                local.get $15
                                local.get $10
                                i64.load
                                i64.store32 offset=48
                                local.get $15
                                i32.const 0
                                i32.store offset=52
                                local.get $10
                                local.get $24
                                i32.store
                                i32.const -1
                                local.set $12
                                br $block_15
                              end ;; $block_20
                              local.get $4
                              if $if_25
                                local.get $4
                                local.set $12
                                br $block_15
                              else
                                local.get $0
                                i32.const 32
                                local.get $16
                                i32.const 0
                                local.get $7
                                call $_pad_524
                                i32.const 0
                                local.set $4
                                br $block_13
                              end ;; $if_25
                              unreachable
                            end ;; $block_19
                            local.get $0
                            local.get $10
                            f64.load
                            local.get $16
                            local.get $4
                            local.get $7
                            local.get $6
                            i32.const 1
                            call_indirect $18 (type $1)
                            local.set $4
                            br $block_9
                          end ;; $block_18
                          local.get $12
                          local.set $6
                          local.get $4
                          local.set $5
                          i32.const 0
                          local.set $12
                          i32.const 2128
                          local.set $8
                          local.get $18
                          local.set $4
                          br $block_12
                        end ;; $block_17
                        local.get $10
                        i64.load
                        local.get $19
                        local.get $6
                        i32.const 32
                        i32.and
                        call $_fmt_x
                        local.set $5
                        i32.const 0
                        i32.const 2
                        local.get $7
                        i32.const 8
                        i32.and
                        i32.eqz
                        local.get $10
                        i64.load
                        i64.const 0
                        i64.eq
                        i32.or
                        local.tee $8
                        select
                        local.set $12
                        i32.const 2128
                        local.get $6
                        i32.const 4
                        i32.shr_u
                        i32.const 2128
                        i32.add
                        local.get $8
                        select
                        local.set $8
                        br $block_14
                      end ;; $block_16
                      local.get $25
                      local.get $19
                      call $_fmt_u
                      local.set $5
                      br $block_14
                    end ;; $block_15
                    i32.const 0
                    local.set $4
                    local.get $10
                    i32.load
                    local.set $6
                    block $block_37
                      block $block_38
                        loop $loop_5
                          local.get $6
                          i32.load
                          local.tee $5
                          if $if_26
                            local.get $20
                            local.get $5
                            call $_wctomb
                            local.tee $5
                            i32.const 0
                            i32.lt_s
                            local.tee $8
                            local.get $5
                            local.get $12
                            local.get $4
                            i32.sub
                            i32.gt_u
                            i32.or
                            br_if $block_38
                            local.get $6
                            i32.const 4
                            i32.add
                            local.set $6
                            local.get $12
                            local.get $4
                            local.get $5
                            i32.add
                            local.tee $4
                            i32.gt_u
                            br_if $loop_5
                          end ;; $if_26
                        end ;; $loop_5
                        br $block_37
                      end ;; $block_38
                      local.get $8
                      if $if_27
                        i32.const -1
                        local.set $9
                        br $block_1
                      end ;; $if_27
                    end ;; $block_37
                    local.get $0
                    i32.const 32
                    local.get $16
                    local.get $4
                    local.get $7
                    call $_pad_524
                    local.get $4
                    if $if_28
                      i32.const 0
                      local.set $12
                      local.get $10
                      i32.load
                      local.set $6
                      loop $loop_6
                        local.get $6
                        i32.load
                        local.tee $5
                        i32.eqz
                        br_if $block_13
                        local.get $20
                        local.get $5
                        call $_wctomb
                        local.tee $5
                        local.get $12
                        i32.add
                        local.tee $12
                        local.get $4
                        i32.gt_s
                        br_if $block_13
                        local.get $6
                        i32.const 4
                        i32.add
                        local.set $6
                        local.get $0
                        local.get $20
                        local.get $5
                        call $_out
                        local.get $12
                        local.get $4
                        i32.lt_u
                        br_if $loop_6
                      end ;; $loop_6
                    else
                      i32.const 0
                      local.set $4
                    end ;; $if_28
                    br $block_13
                  end ;; $block_14
                  local.get $5
                  local.get $19
                  local.get $10
                  i64.load
                  i64.const 0
                  i64.ne
                  local.tee $13
                  local.get $4
                  i32.const 0
                  i32.ne
                  i32.or
                  local.tee $14
                  select
                  local.set $6
                  local.get $7
                  i32.const -65537
                  i32.and
                  local.get $7
                  local.get $4
                  i32.const -1
                  i32.gt_s
                  select
                  local.set $7
                  local.get $4
                  local.get $18
                  local.get $5
                  i32.sub
                  local.get $13
                  i32.const 1
                  i32.xor
                  i32.add
                  local.tee $5
                  local.get $4
                  local.get $5
                  i32.gt_s
                  select
                  i32.const 0
                  local.get $14
                  select
                  local.set $5
                  local.get $18
                  local.set $4
                  br $block_12
                end ;; $block_13
                local.get $0
                i32.const 32
                local.get $16
                local.get $4
                local.get $7
                i32.const 8192
                i32.xor
                call $_pad_524
                local.get $16
                local.get $4
                local.get $16
                local.get $4
                i32.gt_s
                select
                local.set $4
                br $block_9
              end ;; $block_12
              local.get $0
              i32.const 32
              local.get $12
              local.get $4
              local.get $6
              i32.sub
              local.tee $13
              local.get $5
              local.get $5
              local.get $13
              i32.lt_s
              select
              local.tee $14
              i32.add
              local.tee $5
              local.get $16
              local.get $16
              local.get $5
              i32.lt_s
              select
              local.tee $4
              local.get $5
              local.get $7
              call $_pad_524
              local.get $0
              local.get $8
              local.get $12
              call $_out
              local.get $0
              i32.const 48
              local.get $4
              local.get $5
              local.get $7
              i32.const 65536
              i32.xor
              call $_pad_524
              local.get $0
              i32.const 48
              local.get $14
              local.get $13
              i32.const 0
              call $_pad_524
              local.get $0
              local.get $6
              local.get $13
              call $_out
              local.get $0
              i32.const 32
              local.get $4
              local.get $5
              local.get $7
              i32.const 8192
              i32.xor
              call $_pad_524
            end ;; $block_9
            local.get $22
            local.set $7
            br $loop
          end ;; $block_1
        end ;; $loop
        br $block
      end ;; $block_0
      local.get $0
      i32.eqz
      if $if_29
        local.get $7
        if $if_30 (result i32)
          i32.const 1
          local.set $0
          loop $loop_7
            local.get $0
            i32.const 2
            i32.shl
            local.get $3
            i32.add
            i32.load
            local.tee $4
            if $if_31
              local.get $0
              i32.const 3
              i32.shl
              local.get $2
              i32.add
              local.get $4
              local.get $1
              call $_pop_arg
              local.get $0
              i32.const 1
              i32.add
              local.tee $0
              i32.const 10
              i32.lt_u
              br_if $loop_7
              i32.const 1
              local.set $9
              br $block
            end ;; $if_31
          end ;; $loop_7
          loop $loop_8 (result i32)
            local.get $0
            i32.const 2
            i32.shl
            local.get $3
            i32.add
            i32.load
            if $if_32
              i32.const -1
              local.set $9
              br $block
            end ;; $if_32
            local.get $0
            i32.const 1
            i32.add
            local.tee $0
            i32.const 10
            i32.lt_u
            br_if $loop_8
            i32.const 1
          end ;; $loop_8
        else
          i32.const 0
        end ;; $if_30
        local.set $9
      end ;; $if_29
    end ;; $block
    local.get $15
    global.set $22
    local.get $9
    )
  
  (func $_out (type $10)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    local.get $0
    i32.load
    i32.const 32
    i32.and
    i32.eqz
    if $if
      local.get $1
      local.get $2
      local.get $0
      call $___fwritex
    end ;; $if
    )
  
  (func $_getint (type $9)
    (param $0 i32)
    (result i32)
    (local $1 i32)
    (local $2 i32)
    local.get $0
    i32.load
    i32.load8_s
    i32.const -48
    i32.add
    i32.const 10
    i32.lt_u
    if $if
      loop $loop
        local.get $0
        i32.load
        local.tee $1
        i32.load8_s
        local.get $2
        i32.const 10
        i32.mul
        i32.const -48
        i32.add
        i32.add
        local.set $2
        local.get $0
        local.get $1
        i32.const 1
        i32.add
        i32.store
        local.get $1
        i32.load8_s offset=1
        i32.const -48
        i32.add
        i32.const 10
        i32.lt_u
        br_if $loop
      end ;; $loop
    end ;; $if
    local.get $2
    )
  
  (func $_pop_arg (type $10)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (local $3 i32)
    (local $4 i64)
    (local $5 f64)
    local.get $1
    i32.const 20
    i32.le_u
    if $if
      block $block
        block $block_0
          block $block_1
            block $block_2
              block $block_3
                block $block_4
                  block $block_5
                    block $block_6
                      block $block_7
                        block $block_8
                          block $block_9
                            local.get $1
                            i32.const 9
                            i32.sub
                            br_table
                              $block_9 $block_8 $block_7 $block_6 $block_5 $block_4 $block_3 $block_2 $block_1 $block_0
                              $block ;; default
                          end ;; $block_9
                          local.get $2
                          i32.load
                          i32.const 3
                          i32.add
                          i32.const -4
                          i32.and
                          local.tee $1
                          i32.load
                          local.set $3
                          local.get $2
                          local.get $1
                          i32.const 4
                          i32.add
                          i32.store
                          local.get $0
                          local.get $3
                          i32.store
                          br $block
                        end ;; $block_8
                        local.get $2
                        i32.load
                        i32.const 3
                        i32.add
                        i32.const -4
                        i32.and
                        local.tee $1
                        i32.load
                        local.set $3
                        local.get $2
                        local.get $1
                        i32.const 4
                        i32.add
                        i32.store
                        local.get $0
                        local.get $3
                        i64.extend_i32_s
                        i64.store
                        br $block
                      end ;; $block_7
                      local.get $2
                      i32.load
                      i32.const 3
                      i32.add
                      i32.const -4
                      i32.and
                      local.tee $1
                      i32.load
                      local.set $3
                      local.get $2
                      local.get $1
                      i32.const 4
                      i32.add
                      i32.store
                      local.get $0
                      local.get $3
                      i64.extend_i32_u
                      i64.store
                      br $block
                    end ;; $block_6
                    local.get $2
                    i32.load
                    i32.const 7
                    i32.add
                    i32.const -8
                    i32.and
                    local.tee $1
                    i64.load
                    local.set $4
                    local.get $2
                    local.get $1
                    i32.const 8
                    i32.add
                    i32.store
                    local.get $0
                    local.get $4
                    i64.store
                    br $block
                  end ;; $block_5
                  local.get $2
                  i32.load
                  i32.const 3
                  i32.add
                  i32.const -4
                  i32.and
                  local.tee $1
                  i32.load
                  local.set $3
                  local.get $2
                  local.get $1
                  i32.const 4
                  i32.add
                  i32.store
                  local.get $0
                  local.get $3
                  i32.const 65535
                  i32.and
                  i32.const 16
                  i32.shl
                  i32.const 16
                  i32.shr_s
                  i64.extend_i32_s
                  i64.store
                  br $block
                end ;; $block_4
                local.get $2
                i32.load
                i32.const 3
                i32.add
                i32.const -4
                i32.and
                local.tee $1
                i32.load
                local.set $3
                local.get $2
                local.get $1
                i32.const 4
                i32.add
                i32.store
                local.get $0
                local.get $3
                i32.const 65535
                i32.and
                i64.extend_i32_u
                i64.store
                br $block
              end ;; $block_3
              local.get $2
              i32.load
              i32.const 3
              i32.add
              i32.const -4
              i32.and
              local.tee $1
              i32.load
              local.set $3
              local.get $2
              local.get $1
              i32.const 4
              i32.add
              i32.store
              local.get $0
              local.get $3
              i32.const 255
              i32.and
              i32.const 24
              i32.shl
              i32.const 24
              i32.shr_s
              i64.extend_i32_s
              i64.store
              br $block
            end ;; $block_2
            local.get $2
            i32.load
            i32.const 3
            i32.add
            i32.const -4
            i32.and
            local.tee $1
            i32.load
            local.set $3
            local.get $2
            local.get $1
            i32.const 4
            i32.add
            i32.store
            local.get $0
            local.get $3
            i32.const 255
            i32.and
            i64.extend_i32_u
            i64.store
            br $block
          end ;; $block_1
          local.get $2
          i32.load
          i32.const 7
          i32.add
          i32.const -8
          i32.and
          local.tee $1
          f64.load
          local.set $5
          local.get $2
          local.get $1
          i32.const 8
          i32.add
          i32.store
          local.get $0
          local.get $5
          f64.store
          br $block
        end ;; $block_0
        local.get $0
        local.get $2
        i32.const 16
        call_indirect $18 (type $2)
      end ;; $block
    end ;; $if
    )
  
  (func $_fmt_x (type $14)
    (param $0 i64)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    local.get $0
    i64.const 0
    i64.ne
    if $if
      loop $loop
        local.get $1
        i32.const -1
        i32.add
        local.tee $1
        local.get $2
        local.get $0
        i32.wrap_i64
        i32.const 15
        i32.and
        i32.const 1488
        i32.add
        i32.load8_u
        i32.or
        i32.store8
        local.get $0
        i64.const 4
        i64.shr_u
        local.tee $0
        i64.const 0
        i64.ne
        br_if $loop
      end ;; $loop
    end ;; $if
    local.get $1
    )
  
  (func $_fmt_o (type $15)
    (param $0 i64)
    (param $1 i32)
    (result i32)
    local.get $0
    i64.const 0
    i64.ne
    if $if
      loop $loop
        local.get $1
        i32.const -1
        i32.add
        local.tee $1
        local.get $0
        i32.wrap_i64
        i32.const 7
        i32.and
        i32.const 48
        i32.or
        i32.store8
        local.get $0
        i64.const 3
        i64.shr_u
        local.tee $0
        i64.const 0
        i64.ne
        br_if $loop
      end ;; $loop
    end ;; $if
    local.get $1
    )
  
  (func $_fmt_u (type $15)
    (param $0 i64)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i64)
    local.get $0
    i32.wrap_i64
    local.set $2
    block $block (result i32)
      local.get $0
      i64.const 4294967295
      i64.gt_u
      if $if
        loop $loop
          local.get $1
          i32.const -1
          i32.add
          local.tee $1
          local.get $0
          local.get $0
          local.tee $4
          i64.const 10
          i64.div_u
          local.tee $0
          i64.const 10
          i64.mul
          i64.sub
          i32.wrap_i64
          i32.const 255
          i32.and
          i32.const 48
          i32.or
          i32.store8
          local.get $4
          i64.const 42949672959
          i64.gt_u
          br_if $loop
        end ;; $loop
        local.get $0
        i32.wrap_i64
        local.set $2
      end ;; $if
      local.get $2
    end ;; $block
    if $if_0
      loop $loop_0
        local.get $1
        i32.const -1
        i32.add
        local.tee $1
        local.get $2
        local.get $2
        local.tee $3
        i32.const 10
        i32.div_u
        local.tee $2
        i32.const 10
        i32.mul
        i32.sub
        i32.const 48
        i32.or
        i32.store8
        local.get $3
        i32.const 10
        i32.ge_u
        br_if $loop_0
      end ;; $loop_0
    end ;; $if_0
    local.get $1
    )
  
  (func $_pad_524 (type $6)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (param $4 i32)
    (local $5 i32)
    global.get $22
    local.set $5
    global.get $22
    i32.const 256
    i32.add
    global.set $22
    local.get $4
    i32.const 73728
    i32.and
    i32.eqz
    local.get $2
    local.get $3
    i32.gt_s
    i32.and
    if $if
      local.get $5
      local.get $1
      i32.const 24
      i32.shl
      i32.const 24
      i32.shr_s
      local.get $2
      local.get $3
      i32.sub
      local.tee $1
      i32.const 256
      local.get $1
      i32.const 256
      i32.lt_u
      select
      call $_memset
      drop
      local.get $1
      i32.const 255
      i32.gt_u
      if $if_0
        local.get $2
        local.get $3
        i32.sub
        local.set $2
        loop $loop
          local.get $0
          local.get $5
          i32.const 256
          call $_out
          local.get $1
          i32.const -256
          i32.add
          local.tee $1
          i32.const 255
          i32.gt_u
          br_if $loop
        end ;; $loop
        local.get $2
        i32.const 255
        i32.and
        local.set $1
      end ;; $if_0
      local.get $0
      local.get $5
      local.get $1
      call $_out
    end ;; $if
    local.get $5
    global.set $22
    )
  
  (func $_wctomb (type $12)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    local.get $0
    if $if (result i32)
      local.get $0
      local.get $1
      call $_wcrtomb
    else
      i32.const 0
    end ;; $if
    )
  
  (func $_wcrtomb (type $12)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    local.get $0
    if $if (result i32)
      block $block (result i32)
        local.get $1
        i32.const 128
        i32.lt_u
        if $if_0
          local.get $0
          local.get $1
          i32.store8
          i32.const 1
          br $block
        end ;; $if_0
        i32.const 1924
        i32.load
        i32.load
        i32.eqz
        if $if_1
          local.get $1
          i32.const -128
          i32.and
          i32.const 57216
          i32.eq
          if $if_2
            local.get $0
            local.get $1
            i32.store8
            i32.const 1
            br $block
          else
            i32.const 2496
            i32.const 84
            i32.store
            i32.const -1
            br $block
          end ;; $if_2
          unreachable
        end ;; $if_1
        local.get $1
        i32.const 2048
        i32.lt_u
        if $if_3
          local.get $0
          local.get $1
          i32.const 6
          i32.shr_u
          i32.const 192
          i32.or
          i32.store8
          local.get $0
          local.get $1
          i32.const 63
          i32.and
          i32.const 128
          i32.or
          i32.store8 offset=1
          i32.const 2
          br $block
        end ;; $if_3
        local.get $1
        i32.const -8192
        i32.and
        i32.const 57344
        i32.eq
        local.get $1
        i32.const 55296
        i32.lt_u
        i32.or
        if $if_4
          local.get $0
          local.get $1
          i32.const 12
          i32.shr_u
          i32.const 224
          i32.or
          i32.store8
          local.get $0
          local.get $1
          i32.const 6
          i32.shr_u
          i32.const 63
          i32.and
          i32.const 128
          i32.or
          i32.store8 offset=1
          local.get $0
          local.get $1
          i32.const 63
          i32.and
          i32.const 128
          i32.or
          i32.store8 offset=2
          i32.const 3
          br $block
        end ;; $if_4
        local.get $1
        i32.const -65536
        i32.add
        i32.const 1048576
        i32.lt_u
        if $if_5 (result i32)
          local.get $0
          local.get $1
          i32.const 18
          i32.shr_u
          i32.const 240
          i32.or
          i32.store8
          local.get $0
          local.get $1
          i32.const 12
          i32.shr_u
          i32.const 63
          i32.and
          i32.const 128
          i32.or
          i32.store8 offset=1
          local.get $0
          local.get $1
          i32.const 6
          i32.shr_u
          i32.const 63
          i32.and
          i32.const 128
          i32.or
          i32.store8 offset=2
          local.get $0
          local.get $1
          i32.const 63
          i32.and
          i32.const 128
          i32.or
          i32.store8 offset=3
          i32.const 4
        else
          i32.const 2496
          i32.const 84
          i32.store
          i32.const -1
        end ;; $if_5
      end ;; $block
    else
      i32.const 1
    end ;; $if
    )
  
  (func $___fwritex (type $10)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    block $block
      block $block_0
        local.get $2
        i32.load offset=16
        local.tee $3
        br_if $block_0
        local.get $2
        call $___towrite
        i32.eqz
        if $if
          local.get $2
          i32.load offset=16
          local.set $3
          br $block_0
        end ;; $if
        br $block
      end ;; $block_0
      local.get $3
      local.get $2
      i32.load offset=20
      local.tee $4
      i32.sub
      local.get $1
      i32.lt_u
      if $if_0
        local.get $2
        local.get $0
        local.get $1
        local.get $2
        i32.load offset=36
        i32.const 3
        i32.and
        i32.const 2
        i32.add
        call_indirect $18 (type $0)
        drop
        br $block
      end ;; $if_0
      local.get $1
      i32.eqz
      local.get $2
      i32.load8_s offset=75
      i32.const 0
      i32.lt_s
      i32.or
      i32.eqz
      if $if_1
        block $block_1
          local.get $1
          local.set $3
          loop $loop
            local.get $0
            local.get $3
            i32.const -1
            i32.add
            local.tee $5
            i32.add
            i32.load8_s
            i32.const 10
            i32.ne
            if $if_2
              local.get $5
              if $if_3
                local.get $5
                local.set $3
                br $loop
              else
                br $block_1
              end ;; $if_3
              unreachable
            end ;; $if_2
          end ;; $loop
          local.get $2
          local.get $0
          local.get $3
          local.get $2
          i32.load offset=36
          i32.const 3
          i32.and
          i32.const 2
          i32.add
          call_indirect $18 (type $0)
          local.get $3
          i32.lt_u
          br_if $block
          local.get $2
          i32.load offset=20
          local.set $4
          local.get $1
          local.get $3
          i32.sub
          local.set $1
          local.get $0
          local.get $3
          i32.add
          local.set $0
        end ;; $block_1
      end ;; $if_1
      local.get $4
      local.get $0
      local.get $1
      call $_memcpy
      drop
      local.get $2
      local.get $2
      i32.load offset=20
      local.get $1
      i32.add
      i32.store offset=20
    end ;; $block
    )
  
  (func $___towrite (type $9)
    (param $0 i32)
    (result i32)
    (local $1 i32)
    local.get $0
    local.get $0
    i32.load8_s offset=74
    local.tee $1
    local.get $1
    i32.const 255
    i32.add
    i32.or
    i32.store8 offset=74
    local.get $0
    i32.load
    local.tee $1
    i32.const 8
    i32.and
    if $if (result i32)
      local.get $0
      local.get $1
      i32.const 32
      i32.or
      i32.store
      i32.const -1
    else
      local.get $0
      i32.const 0
      i32.store offset=8
      local.get $0
      i32.const 0
      i32.store offset=4
      local.get $0
      local.get $0
      i32.load offset=44
      local.tee $1
      i32.store offset=28
      local.get $0
      local.get $1
      i32.store offset=20
      local.get $0
      local.get $1
      local.get $0
      i32.load offset=48
      i32.add
      i32.store offset=16
      i32.const 0
    end ;; $if
    )
  
  (func $_sn_write (type $0)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    (local $3 i32)
    (local $4 i32)
    local.get $2
    local.get $0
    i32.load offset=16
    local.get $0
    i32.load offset=20
    local.tee $4
    i32.sub
    local.tee $3
    local.get $3
    local.get $2
    i32.gt_u
    select
    local.set $3
    local.get $4
    local.get $1
    local.get $3
    call $_memcpy
    drop
    local.get $0
    local.get $0
    i32.load offset=20
    local.get $3
    i32.add
    i32.store offset=20
    local.get $2
    )
  
  (func $_malloc (type $9)
    (param $0 i32)
    (result i32)
    (local $1 i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    (local $7 i32)
    (local $8 i32)
    (local $9 i32)
    (local $10 i32)
    (local $11 i32)
    (local $12 i32)
    (local $13 i32)
    (local $14 i32)
    (local $15 i32)
    (local $16 i32)
    (local $17 i32)
    (local $18 i32)
    (local $19 i32)
    (local $20 i32)
    (local $21 i32)
    (local $22 i32)
    global.get $22
    local.set $17
    global.get $22
    i32.const 16
    i32.add
    global.set $22
    local.get $0
    i32.const 245
    i32.lt_u
    if $if (result i32)
      i32.const 2500
      i32.load
      local.tee $6
      i32.const 16
      local.get $0
      i32.const 11
      i32.add
      i32.const -8
      i32.and
      local.get $0
      i32.const 11
      i32.lt_u
      select
      local.tee $14
      i32.const 3
      i32.shr_u
      local.tee $11
      i32.shr_u
      local.tee $1
      i32.const 3
      i32.and
      if $if_0
        local.get $1
        i32.const 1
        i32.and
        i32.const 1
        i32.xor
        local.get $11
        i32.add
        local.tee $0
        i32.const 3
        i32.shl
        i32.const 2540
        i32.add
        local.tee $3
        i32.load offset=8
        local.tee $2
        i32.const 8
        i32.add
        local.tee $1
        i32.load
        local.tee $4
        local.get $3
        i32.eq
        if $if_1
          i32.const 2500
          local.get $6
          i32.const 1
          local.get $0
          i32.shl
          i32.const -1
          i32.xor
          i32.and
          i32.store
        else
          i32.const 2516
          i32.load
          local.get $4
          i32.gt_u
          if $if_2
            call $_abort
          end ;; $if_2
          local.get $2
          local.get $4
          i32.load offset=12
          i32.eq
          if $if_3
            local.get $4
            local.get $3
            i32.store offset=12
            local.get $3
            local.get $4
            i32.store offset=8
          else
            call $_abort
          end ;; $if_3
        end ;; $if_1
        local.get $2
        local.get $0
        i32.const 3
        i32.shl
        local.tee $0
        i32.const 3
        i32.or
        i32.store offset=4
        local.get $0
        local.get $2
        i32.add
        local.tee $0
        local.get $0
        i32.load offset=4
        i32.const 1
        i32.or
        i32.store offset=4
        local.get $17
        global.set $22
        local.get $1
        return
      end ;; $if_0
      local.get $14
      i32.const 2508
      i32.load
      local.tee $13
      i32.gt_u
      if $if_4 (result i32)
        local.get $1
        if $if_5
          i32.const 2
          local.get $11
          i32.shl
          local.tee $0
          i32.const 0
          local.get $0
          i32.sub
          i32.or
          local.get $1
          local.get $11
          i32.shl
          i32.and
          local.tee $0
          i32.const 0
          local.get $0
          i32.sub
          i32.and
          i32.const -1
          i32.add
          local.tee $1
          i32.const 12
          i32.shr_u
          i32.const 16
          i32.and
          local.tee $0
          local.get $1
          local.get $0
          i32.shr_u
          local.tee $1
          i32.const 5
          i32.shr_u
          i32.const 8
          i32.and
          local.tee $0
          i32.or
          local.get $1
          local.get $0
          i32.shr_u
          local.tee $1
          i32.const 2
          i32.shr_u
          i32.const 4
          i32.and
          local.tee $0
          i32.or
          local.get $1
          local.get $0
          i32.shr_u
          local.tee $1
          i32.const 1
          i32.shr_u
          i32.const 2
          i32.and
          local.tee $0
          i32.or
          local.get $1
          local.get $0
          i32.shr_u
          local.tee $1
          i32.const 1
          i32.shr_u
          i32.const 1
          i32.and
          local.tee $0
          i32.or
          local.get $1
          local.get $0
          i32.shr_u
          i32.add
          local.tee $0
          i32.const 3
          i32.shl
          i32.const 2540
          i32.add
          local.tee $1
          i32.load offset=8
          local.tee $10
          i32.const 8
          i32.add
          local.tee $9
          i32.load
          local.tee $2
          local.get $1
          i32.eq
          if $if_6
            i32.const 2500
            local.get $6
            i32.const 1
            local.get $0
            i32.shl
            i32.const -1
            i32.xor
            i32.and
            local.tee $8
            i32.store
          else
            i32.const 2516
            i32.load
            local.get $2
            i32.gt_u
            if $if_7
              call $_abort
            end ;; $if_7
            local.get $2
            i32.load offset=12
            local.get $10
            i32.eq
            if $if_8
              local.get $2
              local.get $1
              i32.store offset=12
              local.get $1
              local.get $2
              i32.store offset=8
              local.get $6
              local.set $8
            else
              call $_abort
            end ;; $if_8
          end ;; $if_6
          local.get $10
          local.get $14
          i32.const 3
          i32.or
          i32.store offset=4
          local.get $10
          local.get $14
          i32.add
          local.tee $3
          local.get $0
          i32.const 3
          i32.shl
          local.tee $0
          local.get $14
          i32.sub
          local.tee $6
          i32.const 1
          i32.or
          i32.store offset=4
          local.get $0
          local.get $10
          i32.add
          local.get $6
          i32.store
          local.get $13
          if $if_9
            i32.const 2520
            i32.load
            local.set $10
            local.get $13
            i32.const 3
            i32.shr_u
            local.tee $0
            i32.const 3
            i32.shl
            i32.const 2540
            i32.add
            local.set $2
            local.get $8
            i32.const 1
            local.get $0
            i32.shl
            local.tee $0
            i32.and
            if $if_10
              i32.const 2516
              i32.load
              local.get $2
              i32.const 8
              i32.add
              local.tee $1
              i32.load
              local.tee $0
              i32.gt_u
              if $if_11
                call $_abort
              else
                local.get $1
                local.set $7
                local.get $0
                local.set $4
              end ;; $if_11
            else
              i32.const 2500
              local.get $0
              local.get $8
              i32.or
              i32.store
              local.get $2
              i32.const 8
              i32.add
              local.set $7
              local.get $2
              local.set $4
            end ;; $if_10
            local.get $7
            local.get $10
            i32.store
            local.get $4
            local.get $10
            i32.store offset=12
            local.get $10
            local.get $4
            i32.store offset=8
            local.get $10
            local.get $2
            i32.store offset=12
          end ;; $if_9
          i32.const 2508
          local.get $6
          i32.store
          i32.const 2520
          local.get $3
          i32.store
          local.get $17
          global.set $22
          local.get $9
          return
        end ;; $if_5
        i32.const 2504
        i32.load
        local.tee $11
        if $if_12 (result i32)
          local.get $11
          i32.const 0
          local.get $11
          i32.sub
          i32.and
          i32.const -1
          i32.add
          local.tee $1
          i32.const 12
          i32.shr_u
          i32.const 16
          i32.and
          local.tee $0
          local.get $1
          local.get $0
          i32.shr_u
          local.tee $1
          i32.const 5
          i32.shr_u
          i32.const 8
          i32.and
          local.tee $0
          i32.or
          local.get $1
          local.get $0
          i32.shr_u
          local.tee $1
          i32.const 2
          i32.shr_u
          i32.const 4
          i32.and
          local.tee $0
          i32.or
          local.get $1
          local.get $0
          i32.shr_u
          local.tee $1
          i32.const 1
          i32.shr_u
          i32.const 2
          i32.and
          local.tee $0
          i32.or
          local.get $1
          local.get $0
          i32.shr_u
          local.tee $1
          i32.const 1
          i32.shr_u
          i32.const 1
          i32.and
          local.tee $0
          i32.or
          local.get $1
          local.get $0
          i32.shr_u
          i32.add
          i32.const 2
          i32.shl
          i32.const 2804
          i32.add
          i32.load
          local.tee $0
          i32.load offset=4
          i32.const -8
          i32.and
          local.get $14
          i32.sub
          local.set $10
          local.get $0
          local.set $9
          loop $loop
            block $block
              local.get $0
              i32.load offset=16
              local.tee $1
              if $if_13
                local.get $1
                local.set $0
              else
                local.get $0
                i32.load offset=20
                local.tee $0
                i32.eqz
                br_if $block
              end ;; $if_13
              local.get $0
              i32.load offset=4
              i32.const -8
              i32.and
              local.get $14
              i32.sub
              local.tee $1
              local.get $10
              i32.lt_u
              local.set $4
              local.get $1
              local.get $10
              local.get $4
              select
              local.set $10
              local.get $0
              local.get $9
              local.get $4
              select
              local.set $9
              br $loop
            end ;; $block
          end ;; $loop
          i32.const 2516
          i32.load
          local.tee $7
          local.get $9
          i32.gt_u
          if $if_14
            call $_abort
          end ;; $if_14
          local.get $9
          local.get $14
          i32.add
          local.tee $5
          local.get $9
          i32.le_u
          if $if_15
            call $_abort
          end ;; $if_15
          local.get $9
          i32.load offset=24
          local.set $18
          local.get $9
          i32.load offset=12
          local.tee $0
          local.get $9
          i32.eq
          if $if_16
            block $block_0
              local.get $9
              i32.const 20
              i32.add
              local.tee $1
              i32.load
              local.tee $0
              i32.eqz
              if $if_17
                local.get $9
                i32.const 16
                i32.add
                local.tee $1
                i32.load
                local.tee $0
                i32.eqz
                br_if $block_0
              end ;; $if_17
              loop $loop_0
                block $block_1
                  local.get $0
                  i32.const 20
                  i32.add
                  local.tee $8
                  i32.load
                  local.tee $4
                  i32.eqz
                  if $if_18
                    local.get $0
                    i32.const 16
                    i32.add
                    local.tee $8
                    i32.load
                    local.tee $4
                    i32.eqz
                    br_if $block_1
                  end ;; $if_18
                  local.get $8
                  local.set $1
                  local.get $4
                  local.set $0
                  br $loop_0
                end ;; $block_1
              end ;; $loop_0
              local.get $7
              local.get $1
              i32.gt_u
              if $if_19
                call $_abort
              else
                local.get $1
                i32.const 0
                i32.store
                local.get $0
                local.set $3
              end ;; $if_19
            end ;; $block_0
          else
            local.get $7
            local.get $9
            i32.load offset=8
            local.tee $1
            i32.gt_u
            if $if_20
              call $_abort
            end ;; $if_20
            local.get $9
            local.get $1
            i32.load offset=12
            i32.ne
            if $if_21
              call $_abort
            end ;; $if_21
            local.get $0
            i32.load offset=8
            local.get $9
            i32.eq
            if $if_22
              local.get $1
              local.get $0
              i32.store offset=12
              local.get $0
              local.get $1
              i32.store offset=8
              local.get $0
              local.set $3
            else
              call $_abort
            end ;; $if_22
          end ;; $if_16
          local.get $18
          if $if_23
            block $block_2
              local.get $9
              i32.load offset=28
              local.tee $1
              i32.const 2
              i32.shl
              i32.const 2804
              i32.add
              local.tee $0
              i32.load
              local.get $9
              i32.eq
              if $if_24
                local.get $0
                local.get $3
                i32.store
                local.get $3
                i32.eqz
                if $if_25
                  i32.const 2504
                  local.get $11
                  i32.const 1
                  local.get $1
                  i32.shl
                  i32.const -1
                  i32.xor
                  i32.and
                  i32.store
                  br $block_2
                end ;; $if_25
              else
                i32.const 2516
                i32.load
                local.get $18
                i32.gt_u
                if $if_26
                  call $_abort
                else
                  local.get $18
                  i32.const 16
                  i32.add
                  local.get $18
                  i32.const 20
                  i32.add
                  local.get $18
                  i32.load offset=16
                  local.get $9
                  i32.eq
                  select
                  local.get $3
                  i32.store
                  local.get $3
                  i32.eqz
                  br_if $block_2
                end ;; $if_26
              end ;; $if_24
              i32.const 2516
              i32.load
              local.tee $0
              local.get $3
              i32.gt_u
              if $if_27
                call $_abort
              end ;; $if_27
              local.get $3
              local.get $18
              i32.store offset=24
              local.get $9
              i32.load offset=16
              local.tee $1
              if $if_28
                local.get $0
                local.get $1
                i32.gt_u
                if $if_29
                  call $_abort
                else
                  local.get $3
                  local.get $1
                  i32.store offset=16
                  local.get $1
                  local.get $3
                  i32.store offset=24
                end ;; $if_29
              end ;; $if_28
              local.get $9
              i32.load offset=20
              local.tee $0
              if $if_30
                i32.const 2516
                i32.load
                local.get $0
                i32.gt_u
                if $if_31
                  call $_abort
                else
                  local.get $3
                  local.get $0
                  i32.store offset=20
                  local.get $0
                  local.get $3
                  i32.store offset=24
                end ;; $if_31
              end ;; $if_30
            end ;; $block_2
          end ;; $if_23
          local.get $10
          i32.const 16
          i32.lt_u
          if $if_32
            local.get $9
            local.get $10
            local.get $14
            i32.add
            local.tee $0
            i32.const 3
            i32.or
            i32.store offset=4
            local.get $0
            local.get $9
            i32.add
            local.tee $0
            local.get $0
            i32.load offset=4
            i32.const 1
            i32.or
            i32.store offset=4
          else
            local.get $9
            local.get $14
            i32.const 3
            i32.or
            i32.store offset=4
            local.get $5
            local.get $10
            i32.const 1
            i32.or
            i32.store offset=4
            local.get $5
            local.get $10
            i32.add
            local.get $10
            i32.store
            local.get $13
            if $if_33
              i32.const 2520
              i32.load
              local.set $4
              local.get $13
              i32.const 3
              i32.shr_u
              local.tee $0
              i32.const 3
              i32.shl
              i32.const 2540
              i32.add
              local.set $3
              local.get $6
              i32.const 1
              local.get $0
              i32.shl
              local.tee $0
              i32.and
              if $if_34
                i32.const 2516
                i32.load
                local.get $3
                i32.const 8
                i32.add
                local.tee $1
                i32.load
                local.tee $0
                i32.gt_u
                if $if_35
                  call $_abort
                else
                  local.get $1
                  local.set $2
                  local.get $0
                  local.set $12
                end ;; $if_35
              else
                i32.const 2500
                local.get $0
                local.get $6
                i32.or
                i32.store
                local.get $3
                i32.const 8
                i32.add
                local.set $2
                local.get $3
                local.set $12
              end ;; $if_34
              local.get $2
              local.get $4
              i32.store
              local.get $12
              local.get $4
              i32.store offset=12
              local.get $4
              local.get $12
              i32.store offset=8
              local.get $4
              local.get $3
              i32.store offset=12
            end ;; $if_33
            i32.const 2508
            local.get $10
            i32.store
            i32.const 2520
            local.get $5
            i32.store
          end ;; $if_32
          local.get $17
          global.set $22
          local.get $9
          i32.const 8
          i32.add
          return
        else
          local.get $14
        end ;; $if_12
      else
        local.get $14
      end ;; $if_4
    else
      local.get $0
      i32.const -65
      i32.gt_u
      if $if_36 (result i32)
        i32.const -1
      else
        block $block_3 (result i32)
          local.get $0
          i32.const 11
          i32.add
          local.tee $0
          i32.const -8
          i32.and
          local.set $15
          i32.const 2504
          i32.load
          local.tee $4
          if $if_37 (result i32)
            i32.const 0
            local.get $15
            i32.sub
            local.set $3
            block $block_4
              block $block_5
                local.get $0
                i32.const 8
                i32.shr_u
                local.tee $0
                if $if_38 (result i32)
                  local.get $15
                  i32.const 16777215
                  i32.gt_u
                  if $if_39 (result i32)
                    i32.const 31
                  else
                    local.get $0
                    local.get $0
                    i32.const 1048320
                    i32.add
                    i32.const 16
                    i32.shr_u
                    i32.const 8
                    i32.and
                    local.tee $12
                    i32.shl
                    local.tee $0
                    i32.const 520192
                    i32.add
                    i32.const 16
                    i32.shr_u
                    i32.const 4
                    i32.and
                    local.set $8
                    local.get $15
                    i32.const 14
                    local.get $0
                    local.get $8
                    i32.shl
                    local.tee $2
                    i32.const 245760
                    i32.add
                    i32.const 16
                    i32.shr_u
                    i32.const 2
                    i32.and
                    local.tee $0
                    local.get $8
                    local.get $12
                    i32.or
                    i32.or
                    i32.sub
                    local.get $2
                    local.get $0
                    i32.shl
                    i32.const 15
                    i32.shr_u
                    i32.add
                    local.tee $0
                    i32.const 7
                    i32.add
                    i32.shr_u
                    i32.const 1
                    i32.and
                    local.get $0
                    i32.const 1
                    i32.shl
                    i32.or
                  end ;; $if_39
                else
                  i32.const 0
                end ;; $if_38
                local.tee $19
                i32.const 2
                i32.shl
                i32.const 2804
                i32.add
                i32.load
                local.tee $0
                if $if_40
                  local.get $15
                  i32.const 0
                  i32.const 25
                  local.get $19
                  i32.const 1
                  i32.shr_u
                  i32.sub
                  local.get $19
                  i32.const 31
                  i32.eq
                  select
                  i32.shl
                  local.set $12
                  i32.const 0
                  local.set $2
                  loop $loop_1
                    local.get $0
                    i32.load offset=4
                    i32.const -8
                    i32.and
                    local.get $15
                    i32.sub
                    local.tee $8
                    local.get $3
                    i32.lt_u
                    if $if_41
                      local.get $8
                      if $if_42 (result i32)
                        local.get $0
                        local.set $2
                        local.get $8
                      else
                        i32.const 0
                        local.set $3
                        local.get $0
                        local.set $2
                        br $block_5
                      end ;; $if_42
                      local.set $3
                    end ;; $if_41
                    local.get $7
                    local.get $0
                    i32.load offset=20
                    local.tee $7
                    local.get $7
                    i32.eqz
                    local.get $7
                    local.get $0
                    i32.const 16
                    i32.add
                    local.get $12
                    i32.const 31
                    i32.shr_u
                    i32.const 2
                    i32.shl
                    i32.add
                    i32.load
                    local.tee $8
                    i32.eq
                    i32.or
                    select
                    local.set $0
                    local.get $12
                    i32.const 1
                    i32.shl
                    local.set $12
                    local.get $8
                    if $if_43
                      local.get $0
                      local.set $7
                      local.get $8
                      local.set $0
                      br $loop_1
                    end ;; $if_43
                  end ;; $loop_1
                else
                  i32.const 0
                  local.set $0
                  i32.const 0
                  local.set $2
                end ;; $if_40
                local.get $0
                local.get $2
                i32.or
                i32.eqz
                if $if_44
                  local.get $15
                  local.get $4
                  i32.const 2
                  local.get $19
                  i32.shl
                  local.tee $0
                  i32.const 0
                  local.get $0
                  i32.sub
                  i32.or
                  i32.and
                  local.tee $0
                  i32.eqz
                  br_if $block_3
                  drop
                  local.get $0
                  i32.const 0
                  local.get $0
                  i32.sub
                  i32.and
                  i32.const -1
                  i32.add
                  local.tee $2
                  i32.const 12
                  i32.shr_u
                  i32.const 16
                  i32.and
                  local.tee $0
                  local.get $2
                  local.get $0
                  i32.shr_u
                  local.tee $2
                  i32.const 5
                  i32.shr_u
                  i32.const 8
                  i32.and
                  local.tee $0
                  i32.or
                  local.get $2
                  local.get $0
                  i32.shr_u
                  local.tee $2
                  i32.const 2
                  i32.shr_u
                  i32.const 4
                  i32.and
                  local.tee $0
                  i32.or
                  local.get $2
                  local.get $0
                  i32.shr_u
                  local.tee $2
                  i32.const 1
                  i32.shr_u
                  i32.const 2
                  i32.and
                  local.tee $0
                  i32.or
                  local.get $2
                  local.get $0
                  i32.shr_u
                  local.tee $2
                  i32.const 1
                  i32.shr_u
                  i32.const 1
                  i32.and
                  local.tee $0
                  i32.or
                  local.get $2
                  local.get $0
                  i32.shr_u
                  i32.add
                  i32.const 2
                  i32.shl
                  i32.const 2804
                  i32.add
                  i32.load
                  local.set $0
                  i32.const 0
                  local.set $2
                end ;; $if_44
                local.get $0
                br_if $block_5
                local.get $3
                local.set $7
                br $block_4
              end ;; $block_5
              local.get $2
              local.set $12
              loop $loop_2 (result i32)
                local.get $0
                i32.load offset=4
                i32.const -8
                i32.and
                local.get $15
                i32.sub
                local.tee $7
                local.get $3
                i32.lt_u
                local.set $8
                local.get $7
                local.get $3
                local.get $8
                select
                local.set $3
                local.get $0
                local.get $12
                local.get $8
                select
                local.set $12
                block $block_6 (result i32)
                  local.get $0
                  i32.load offset=16
                  local.tee $2
                  i32.eqz
                  if $if_45
                    local.get $0
                    i32.load offset=20
                    local.set $2
                  end ;; $if_45
                  local.get $2
                end ;; $block_6
                if $if_46 (result i32)
                  local.get $2
                  local.set $0
                  br $loop_2
                else
                  local.get $3
                  local.set $7
                  local.get $12
                end ;; $if_46
              end ;; $loop_2
              local.set $2
            end ;; $block_4
            local.get $2
            if $if_47 (result i32)
              local.get $7
              i32.const 2508
              i32.load
              local.get $15
              i32.sub
              i32.lt_u
              if $if_48 (result i32)
                i32.const 2516
                i32.load
                local.tee $12
                local.get $2
                i32.gt_u
                if $if_49
                  call $_abort
                end ;; $if_49
                local.get $2
                local.get $15
                i32.add
                local.tee $5
                local.get $2
                i32.le_u
                if $if_50
                  call $_abort
                end ;; $if_50
                local.get $2
                i32.load offset=24
                local.set $8
                local.get $2
                i32.load offset=12
                local.tee $0
                local.get $2
                i32.eq
                if $if_51
                  block $block_7
                    local.get $2
                    i32.const 20
                    i32.add
                    local.tee $3
                    i32.load
                    local.tee $0
                    i32.eqz
                    if $if_52
                      local.get $2
                      i32.const 16
                      i32.add
                      local.tee $3
                      i32.load
                      local.tee $0
                      i32.eqz
                      br_if $block_7
                    end ;; $if_52
                    loop $loop_3
                      block $block_8
                        local.get $0
                        i32.const 20
                        i32.add
                        local.tee $9
                        i32.load
                        local.tee $10
                        i32.eqz
                        if $if_53
                          local.get $0
                          i32.const 16
                          i32.add
                          local.tee $9
                          i32.load
                          local.tee $10
                          i32.eqz
                          br_if $block_8
                        end ;; $if_53
                        local.get $9
                        local.set $3
                        local.get $10
                        local.set $0
                        br $loop_3
                      end ;; $block_8
                    end ;; $loop_3
                    local.get $12
                    local.get $3
                    i32.gt_u
                    if $if_54
                      call $_abort
                    else
                      local.get $3
                      i32.const 0
                      i32.store
                      local.get $0
                      local.set $13
                    end ;; $if_54
                  end ;; $block_7
                else
                  local.get $12
                  local.get $2
                  i32.load offset=8
                  local.tee $3
                  i32.gt_u
                  if $if_55
                    call $_abort
                  end ;; $if_55
                  local.get $2
                  local.get $3
                  i32.load offset=12
                  i32.ne
                  if $if_56
                    call $_abort
                  end ;; $if_56
                  local.get $0
                  i32.load offset=8
                  local.get $2
                  i32.eq
                  if $if_57
                    local.get $3
                    local.get $0
                    i32.store offset=12
                    local.get $0
                    local.get $3
                    i32.store offset=8
                    local.get $0
                    local.set $13
                  else
                    call $_abort
                  end ;; $if_57
                end ;; $if_51
                local.get $8
                if $if_58
                  block $block_9
                    local.get $2
                    i32.load offset=28
                    local.tee $3
                    i32.const 2
                    i32.shl
                    i32.const 2804
                    i32.add
                    local.tee $0
                    i32.load
                    local.get $2
                    i32.eq
                    if $if_59
                      local.get $0
                      local.get $13
                      i32.store
                      local.get $13
                      i32.eqz
                      if $if_60
                        i32.const 2504
                        local.get $4
                        i32.const 1
                        local.get $3
                        i32.shl
                        i32.const -1
                        i32.xor
                        i32.and
                        local.tee $1
                        i32.store
                        br $block_9
                      end ;; $if_60
                    else
                      i32.const 2516
                      i32.load
                      local.get $8
                      i32.gt_u
                      if $if_61
                        call $_abort
                      else
                        local.get $8
                        i32.const 16
                        i32.add
                        local.get $8
                        i32.const 20
                        i32.add
                        local.get $8
                        i32.load offset=16
                        local.get $2
                        i32.eq
                        select
                        local.get $13
                        i32.store
                        local.get $13
                        i32.eqz
                        if $if_62
                          local.get $4
                          local.set $1
                          br $block_9
                        end ;; $if_62
                      end ;; $if_61
                    end ;; $if_59
                    i32.const 2516
                    i32.load
                    local.tee $0
                    local.get $13
                    i32.gt_u
                    if $if_63
                      call $_abort
                    end ;; $if_63
                    local.get $13
                    local.get $8
                    i32.store offset=24
                    local.get $2
                    i32.load offset=16
                    local.tee $3
                    if $if_64
                      local.get $0
                      local.get $3
                      i32.gt_u
                      if $if_65
                        call $_abort
                      else
                        local.get $13
                        local.get $3
                        i32.store offset=16
                        local.get $3
                        local.get $13
                        i32.store offset=24
                      end ;; $if_65
                    end ;; $if_64
                    local.get $2
                    i32.load offset=20
                    local.tee $0
                    if $if_66
                      i32.const 2516
                      i32.load
                      local.get $0
                      i32.gt_u
                      if $if_67
                        call $_abort
                      else
                        local.get $13
                        local.get $0
                        i32.store offset=20
                        local.get $0
                        local.get $13
                        i32.store offset=24
                        local.get $4
                        local.set $1
                      end ;; $if_67
                    else
                      local.get $4
                      local.set $1
                    end ;; $if_66
                  end ;; $block_9
                else
                  local.get $4
                  local.set $1
                end ;; $if_58
                local.get $7
                i32.const 16
                i32.lt_u
                if $if_68
                  local.get $2
                  local.get $7
                  local.get $15
                  i32.add
                  local.tee $0
                  i32.const 3
                  i32.or
                  i32.store offset=4
                  local.get $0
                  local.get $2
                  i32.add
                  local.tee $0
                  local.get $0
                  i32.load offset=4
                  i32.const 1
                  i32.or
                  i32.store offset=4
                else
                  block $block_10
                    local.get $2
                    local.get $15
                    i32.const 3
                    i32.or
                    i32.store offset=4
                    local.get $5
                    local.get $7
                    i32.const 1
                    i32.or
                    i32.store offset=4
                    local.get $5
                    local.get $7
                    i32.add
                    local.get $7
                    i32.store
                    local.get $7
                    i32.const 3
                    i32.shr_u
                    local.set $0
                    local.get $7
                    i32.const 256
                    i32.lt_u
                    if $if_69
                      local.get $0
                      i32.const 3
                      i32.shl
                      i32.const 2540
                      i32.add
                      local.set $3
                      i32.const 2500
                      i32.load
                      local.tee $1
                      i32.const 1
                      local.get $0
                      i32.shl
                      local.tee $0
                      i32.and
                      if $if_70
                        i32.const 2516
                        i32.load
                        local.get $3
                        i32.const 8
                        i32.add
                        local.tee $1
                        i32.load
                        local.tee $0
                        i32.gt_u
                        if $if_71
                          call $_abort
                        else
                          local.get $1
                          local.set $14
                          local.get $0
                          local.set $11
                        end ;; $if_71
                      else
                        i32.const 2500
                        local.get $0
                        local.get $1
                        i32.or
                        i32.store
                        local.get $3
                        i32.const 8
                        i32.add
                        local.set $14
                        local.get $3
                        local.set $11
                      end ;; $if_70
                      local.get $14
                      local.get $5
                      i32.store
                      local.get $11
                      local.get $5
                      i32.store offset=12
                      local.get $5
                      local.get $11
                      i32.store offset=8
                      local.get $5
                      local.get $3
                      i32.store offset=12
                      br $block_10
                    end ;; $if_69
                    local.get $7
                    i32.const 8
                    i32.shr_u
                    local.tee $0
                    if $if_72 (result i32)
                      local.get $7
                      i32.const 16777215
                      i32.gt_u
                      if $if_73 (result i32)
                        i32.const 31
                      else
                        local.get $0
                        local.get $0
                        i32.const 1048320
                        i32.add
                        i32.const 16
                        i32.shr_u
                        i32.const 8
                        i32.and
                        local.tee $4
                        i32.shl
                        local.tee $0
                        i32.const 520192
                        i32.add
                        i32.const 16
                        i32.shr_u
                        i32.const 4
                        i32.and
                        local.set $9
                        local.get $7
                        i32.const 14
                        local.get $0
                        local.get $9
                        i32.shl
                        local.tee $3
                        i32.const 245760
                        i32.add
                        i32.const 16
                        i32.shr_u
                        i32.const 2
                        i32.and
                        local.tee $0
                        local.get $4
                        local.get $9
                        i32.or
                        i32.or
                        i32.sub
                        local.get $3
                        local.get $0
                        i32.shl
                        i32.const 15
                        i32.shr_u
                        i32.add
                        local.tee $0
                        i32.const 7
                        i32.add
                        i32.shr_u
                        i32.const 1
                        i32.and
                        local.get $0
                        i32.const 1
                        i32.shl
                        i32.or
                      end ;; $if_73
                    else
                      i32.const 0
                    end ;; $if_72
                    local.tee $4
                    i32.const 2
                    i32.shl
                    i32.const 2804
                    i32.add
                    local.set $3
                    local.get $5
                    local.get $4
                    i32.store offset=28
                    local.get $5
                    i32.const 0
                    i32.store offset=20
                    local.get $5
                    i32.const 0
                    i32.store offset=16
                    local.get $1
                    i32.const 1
                    local.get $4
                    i32.shl
                    local.tee $0
                    i32.and
                    i32.eqz
                    if $if_74
                      i32.const 2504
                      local.get $0
                      local.get $1
                      i32.or
                      i32.store
                      local.get $3
                      local.get $5
                      i32.store
                      local.get $5
                      local.get $3
                      i32.store offset=24
                      local.get $5
                      local.get $5
                      i32.store offset=12
                      local.get $5
                      local.get $5
                      i32.store offset=8
                      br $block_10
                    end ;; $if_74
                    local.get $3
                    i32.load
                    local.tee $0
                    i32.load offset=4
                    i32.const -8
                    i32.and
                    local.get $7
                    i32.eq
                    if $if_75
                      local.get $0
                      local.set $6
                    else
                      block $block_11
                        local.get $7
                        i32.const 0
                        i32.const 25
                        local.get $4
                        i32.const 1
                        i32.shr_u
                        i32.sub
                        local.get $4
                        i32.const 31
                        i32.eq
                        select
                        i32.shl
                        local.set $3
                        loop $loop_4
                          local.get $0
                          i32.const 16
                          i32.add
                          local.get $3
                          i32.const 31
                          i32.shr_u
                          i32.const 2
                          i32.shl
                          i32.add
                          local.tee $4
                          i32.load
                          local.tee $1
                          if $if_76
                            local.get $3
                            i32.const 1
                            i32.shl
                            local.set $3
                            local.get $1
                            i32.load offset=4
                            i32.const -8
                            i32.and
                            local.get $7
                            i32.eq
                            if $if_77
                              local.get $1
                              local.set $6
                              br $block_11
                            else
                              local.get $1
                              local.set $0
                              br $loop_4
                            end ;; $if_77
                            unreachable
                          end ;; $if_76
                        end ;; $loop_4
                        i32.const 2516
                        i32.load
                        local.get $4
                        i32.gt_u
                        if $if_78
                          call $_abort
                        else
                          local.get $4
                          local.get $5
                          i32.store
                          local.get $5
                          local.get $0
                          i32.store offset=24
                          local.get $5
                          local.get $5
                          i32.store offset=12
                          local.get $5
                          local.get $5
                          i32.store offset=8
                          br $block_10
                        end ;; $if_78
                      end ;; $block_11
                    end ;; $if_75
                    i32.const 2516
                    i32.load
                    local.tee $0
                    local.get $6
                    i32.le_u
                    local.get $0
                    local.get $6
                    i32.load offset=8
                    local.tee $0
                    i32.le_u
                    i32.and
                    if $if_79
                      local.get $0
                      local.get $5
                      i32.store offset=12
                      local.get $6
                      local.get $5
                      i32.store offset=8
                      local.get $5
                      local.get $0
                      i32.store offset=8
                      local.get $5
                      local.get $6
                      i32.store offset=12
                      local.get $5
                      i32.const 0
                      i32.store offset=24
                    else
                      call $_abort
                    end ;; $if_79
                  end ;; $block_10
                end ;; $if_68
                local.get $17
                global.set $22
                local.get $2
                i32.const 8
                i32.add
                return
              else
                local.get $15
              end ;; $if_48
            else
              local.get $15
            end ;; $if_47
          else
            local.get $15
          end ;; $if_37
        end ;; $block_3
      end ;; $if_36
    end ;; $if
    local.set $11
    block $block_12
      block $block_13
        i32.const 2508
        i32.load
        local.tee $3
        local.get $11
        i32.ge_u
        if $if_80
          i32.const 2520
          i32.load
          local.set $0
          local.get $3
          local.get $11
          i32.sub
          local.tee $2
          i32.const 15
          i32.gt_u
          if $if_81
            i32.const 2520
            local.get $0
            local.get $11
            i32.add
            local.tee $1
            i32.store
            i32.const 2508
            local.get $2
            i32.store
            local.get $1
            local.get $2
            i32.const 1
            i32.or
            i32.store offset=4
            local.get $0
            local.get $3
            i32.add
            local.get $2
            i32.store
            local.get $0
            local.get $11
            i32.const 3
            i32.or
            i32.store offset=4
          else
            i32.const 2508
            i32.const 0
            i32.store
            i32.const 2520
            i32.const 0
            i32.store
            local.get $0
            local.get $3
            i32.const 3
            i32.or
            i32.store offset=4
            local.get $0
            local.get $3
            i32.add
            local.tee $1
            local.get $1
            i32.load offset=4
            i32.const 1
            i32.or
            i32.store offset=4
          end ;; $if_81
          br $block_13
        end ;; $if_80
        block $block_14
          i32.const 2512
          i32.load
          local.tee $12
          local.get $11
          i32.gt_u
          if $if_82
            i32.const 2512
            local.get $12
            local.get $11
            i32.sub
            local.tee $3
            i32.store
            br $block_14
          end ;; $if_82
          i32.const 2972
          i32.load
          if $if_83 (result i32)
            i32.const 2980
            i32.load
          else
            i32.const 2980
            i32.const 4096
            i32.store
            i32.const 2976
            i32.const 4096
            i32.store
            i32.const 2984
            i32.const -1
            i32.store
            i32.const 2988
            i32.const -1
            i32.store
            i32.const 2992
            i32.const 0
            i32.store
            i32.const 2944
            i32.const 0
            i32.store
            i32.const 2972
            local.get $17
            i32.const -16
            i32.and
            i32.const 1431655768
            i32.xor
            i32.store
            i32.const 4096
          end ;; $if_83
          local.tee $0
          local.get $11
          i32.const 47
          i32.add
          local.tee $6
          i32.add
          local.tee $4
          i32.const 0
          local.get $0
          i32.sub
          local.tee $3
          i32.and
          local.tee $8
          local.get $11
          i32.le_u
          if $if_84
            br $block_12
          end ;; $if_84
          i32.const 2940
          i32.load
          local.tee $2
          if $if_85
            i32.const 2932
            i32.load
            local.tee $1
            local.get $8
            i32.add
            local.tee $0
            local.get $1
            i32.le_u
            local.get $0
            local.get $2
            i32.gt_u
            i32.or
            if $if_86
              br $block_12
            end ;; $if_86
          end ;; $if_85
          local.get $11
          i32.const 48
          i32.add
          local.set $7
          block $block_15
            block $block_16
              i32.const 2944
              i32.load
              i32.const 4
              i32.and
              if $if_87
                i32.const 0
                local.set $3
              else
                block $block_17
                  block $block_18
                    block $block_19
                      i32.const 2524
                      i32.load
                      local.tee $1
                      i32.eqz
                      br_if $block_19
                      i32.const 2948
                      local.set $2
                      loop $loop_5
                        block $block_20
                          local.get $2
                          i32.load
                          local.tee $0
                          local.get $1
                          i32.le_u
                          if $if_88
                            local.get $0
                            local.get $2
                            i32.load offset=4
                            i32.add
                            local.get $1
                            i32.gt_u
                            br_if $block_20
                          end ;; $if_88
                          local.get $2
                          i32.load offset=8
                          local.tee $2
                          br_if $loop_5
                          br $block_19
                        end ;; $block_20
                      end ;; $loop_5
                      local.get $4
                      local.get $12
                      i32.sub
                      local.get $3
                      i32.and
                      local.tee $3
                      i32.const 2147483647
                      i32.lt_u
                      if $if_89
                        local.get $3
                        call $_sbrk
                        local.set $0
                        local.get $0
                        local.get $2
                        i32.load
                        local.get $2
                        i32.load offset=4
                        i32.add
                        i32.ne
                        br_if $block_18
                        local.get $0
                        i32.const -1
                        i32.ne
                        br_if $block_16
                      else
                        i32.const 0
                        local.set $3
                      end ;; $if_89
                      br $block_17
                    end ;; $block_19
                    i32.const 0
                    call $_sbrk
                    local.tee $0
                    i32.const -1
                    i32.eq
                    if $if_90 (result i32)
                      i32.const 0
                    else
                      i32.const 2932
                      i32.load
                      local.tee $4
                      local.get $0
                      i32.const 2976
                      i32.load
                      local.tee $2
                      i32.const -1
                      i32.add
                      local.tee $1
                      i32.add
                      i32.const 0
                      local.get $2
                      i32.sub
                      i32.and
                      local.get $0
                      i32.sub
                      i32.const 0
                      local.get $0
                      local.get $1
                      i32.and
                      select
                      local.get $8
                      i32.add
                      local.tee $3
                      i32.add
                      local.set $2
                      local.get $3
                      i32.const 2147483647
                      i32.lt_u
                      local.get $3
                      local.get $11
                      i32.gt_u
                      i32.and
                      if $if_91 (result i32)
                        i32.const 2940
                        i32.load
                        local.tee $1
                        if $if_92
                          local.get $2
                          local.get $4
                          i32.le_u
                          local.get $2
                          local.get $1
                          i32.gt_u
                          i32.or
                          if $if_93
                            i32.const 0
                            local.set $3
                            br $block_17
                          end ;; $if_93
                        end ;; $if_92
                        local.get $0
                        local.get $3
                        call $_sbrk
                        local.tee $1
                        i32.eq
                        br_if $block_16
                        local.get $1
                        local.set $0
                        br $block_18
                      else
                        i32.const 0
                      end ;; $if_91
                    end ;; $if_90
                    local.set $3
                    br $block_17
                  end ;; $block_18
                  local.get $0
                  i32.const -1
                  i32.ne
                  local.get $3
                  i32.const 2147483647
                  i32.lt_u
                  i32.and
                  local.get $7
                  local.get $3
                  i32.gt_u
                  i32.and
                  i32.eqz
                  if $if_94
                    local.get $0
                    i32.const -1
                    i32.eq
                    if $if_95
                      i32.const 0
                      local.set $3
                      br $block_17
                    else
                      br $block_16
                    end ;; $if_95
                    unreachable
                  end ;; $if_94
                  i32.const 2980
                  i32.load
                  local.tee $1
                  local.get $6
                  local.get $3
                  i32.sub
                  i32.add
                  i32.const 0
                  local.get $1
                  i32.sub
                  i32.and
                  local.tee $2
                  i32.const 2147483647
                  i32.ge_u
                  br_if $block_16
                  i32.const 0
                  local.get $3
                  i32.sub
                  local.set $1
                  local.get $2
                  call $_sbrk
                  i32.const -1
                  i32.eq
                  if $if_96 (result i32)
                    local.get $1
                    call $_sbrk
                    drop
                    i32.const 0
                  else
                    local.get $2
                    local.get $3
                    i32.add
                    local.set $3
                    br $block_16
                  end ;; $if_96
                  local.set $3
                end ;; $block_17
                i32.const 2944
                i32.const 2944
                i32.load
                i32.const 4
                i32.or
                i32.store
              end ;; $if_87
              local.get $8
              i32.const 2147483647
              i32.lt_u
              if $if_97
                local.get $8
                call $_sbrk
                local.set $0
                i32.const 0
                call $_sbrk
                local.tee $4
                local.get $0
                i32.sub
                local.tee $1
                local.get $11
                i32.const 40
                i32.add
                i32.gt_u
                local.set $2
                local.get $1
                local.get $3
                local.get $2
                select
                local.set $3
                local.get $2
                i32.const 1
                i32.xor
                local.get $0
                i32.const -1
                i32.eq
                i32.or
                local.get $0
                i32.const -1
                i32.ne
                local.get $4
                i32.const -1
                i32.ne
                i32.and
                local.get $0
                local.get $4
                i32.lt_u
                i32.and
                i32.const 1
                i32.xor
                i32.or
                i32.eqz
                br_if $block_16
              end ;; $if_97
              br $block_15
            end ;; $block_16
            i32.const 2932
            i32.const 2932
            i32.load
            local.get $3
            i32.add
            local.tee $1
            i32.store
            local.get $1
            i32.const 2936
            i32.load
            i32.gt_u
            if $if_98
              i32.const 2936
              local.get $1
              i32.store
            end ;; $if_98
            i32.const 2524
            i32.load
            local.tee $6
            if $if_99
              block $block_21
                i32.const 2948
                local.set $2
                block $block_22
                  block $block_23
                    loop $loop_6
                      local.get $2
                      i32.load
                      local.tee $4
                      local.get $2
                      i32.load offset=4
                      local.tee $1
                      i32.add
                      local.get $0
                      i32.eq
                      br_if $block_23
                      local.get $2
                      i32.load offset=8
                      local.tee $2
                      br_if $loop_6
                    end ;; $loop_6
                    br $block_22
                  end ;; $block_23
                  local.get $2
                  i32.load offset=12
                  i32.const 8
                  i32.and
                  i32.eqz
                  if $if_100
                    local.get $4
                    local.get $6
                    i32.le_u
                    local.get $0
                    local.get $6
                    i32.gt_u
                    i32.and
                    if $if_101
                      local.get $2
                      local.get $1
                      local.get $3
                      i32.add
                      i32.store offset=4
                      local.get $6
                      i32.const 0
                      local.get $6
                      i32.const 8
                      i32.add
                      local.tee $0
                      i32.sub
                      i32.const 7
                      i32.and
                      i32.const 0
                      local.get $0
                      i32.const 7
                      i32.and
                      select
                      local.tee $1
                      i32.add
                      local.set $2
                      i32.const 2512
                      i32.load
                      local.get $3
                      i32.add
                      local.tee $0
                      local.get $1
                      i32.sub
                      local.set $1
                      i32.const 2524
                      local.get $2
                      i32.store
                      i32.const 2512
                      local.get $1
                      i32.store
                      local.get $2
                      local.get $1
                      i32.const 1
                      i32.or
                      i32.store offset=4
                      local.get $0
                      local.get $6
                      i32.add
                      i32.const 40
                      i32.store offset=4
                      i32.const 2528
                      i32.const 2988
                      i32.load
                      i32.store
                      br $block_21
                    end ;; $if_101
                  end ;; $if_100
                end ;; $block_22
                local.get $0
                i32.const 2516
                i32.load
                local.tee $2
                i32.lt_u
                if $if_102
                  i32.const 2516
                  local.get $0
                  i32.store
                  local.get $0
                  local.set $2
                end ;; $if_102
                local.get $0
                local.get $3
                i32.add
                local.set $1
                i32.const 2948
                local.set $8
                block $block_24
                  block $block_25
                    loop $loop_7
                      local.get $8
                      i32.load
                      local.get $1
                      i32.eq
                      br_if $block_25
                      local.get $8
                      i32.load offset=8
                      local.tee $8
                      br_if $loop_7
                    end ;; $loop_7
                    br $block_24
                  end ;; $block_25
                  local.get $8
                  i32.load offset=12
                  i32.const 8
                  i32.and
                  i32.eqz
                  if $if_103
                    local.get $8
                    local.get $0
                    i32.store
                    local.get $8
                    local.get $8
                    i32.load offset=4
                    local.get $3
                    i32.add
                    i32.store offset=4
                    local.get $0
                    i32.const 0
                    local.get $0
                    i32.const 8
                    i32.add
                    local.tee $0
                    i32.sub
                    i32.const 7
                    i32.and
                    i32.const 0
                    local.get $0
                    i32.const 7
                    i32.and
                    select
                    i32.add
                    local.tee $12
                    local.get $11
                    i32.add
                    local.set $5
                    local.get $1
                    i32.const 0
                    local.get $1
                    i32.const 8
                    i32.add
                    local.tee $0
                    i32.sub
                    i32.const 7
                    i32.and
                    i32.const 0
                    local.get $0
                    i32.const 7
                    i32.and
                    select
                    i32.add
                    local.tee $3
                    local.get $12
                    i32.sub
                    local.get $11
                    i32.sub
                    local.set $7
                    local.get $12
                    local.get $11
                    i32.const 3
                    i32.or
                    i32.store offset=4
                    local.get $3
                    local.get $6
                    i32.eq
                    if $if_104
                      i32.const 2512
                      i32.const 2512
                      i32.load
                      local.get $7
                      i32.add
                      local.tee $0
                      i32.store
                      i32.const 2524
                      local.get $5
                      i32.store
                      local.get $5
                      local.get $0
                      i32.const 1
                      i32.or
                      i32.store offset=4
                    else
                      block $block_26
                        i32.const 2520
                        i32.load
                        local.get $3
                        i32.eq
                        if $if_105
                          i32.const 2508
                          i32.const 2508
                          i32.load
                          local.get $7
                          i32.add
                          local.tee $0
                          i32.store
                          i32.const 2520
                          local.get $5
                          i32.store
                          local.get $5
                          local.get $0
                          i32.const 1
                          i32.or
                          i32.store offset=4
                          local.get $0
                          local.get $5
                          i32.add
                          local.get $0
                          i32.store
                          br $block_26
                        end ;; $if_105
                        local.get $3
                        i32.load offset=4
                        local.tee $0
                        i32.const 3
                        i32.and
                        i32.const 1
                        i32.eq
                        if $if_106
                          local.get $0
                          i32.const -8
                          i32.and
                          local.set $10
                          local.get $0
                          i32.const 3
                          i32.shr_u
                          local.set $1
                          block $block_27
                            local.get $0
                            i32.const 256
                            i32.lt_u
                            if $if_107
                              local.get $3
                              i32.load offset=12
                              local.set $6
                              local.get $3
                              i32.load offset=8
                              local.tee $4
                              local.get $1
                              i32.const 3
                              i32.shl
                              i32.const 2540
                              i32.add
                              local.tee $0
                              i32.ne
                              if $if_108
                                block $block_28
                                  local.get $2
                                  local.get $4
                                  i32.gt_u
                                  if $if_109
                                    call $_abort
                                  end ;; $if_109
                                  local.get $4
                                  i32.load offset=12
                                  local.get $3
                                  i32.eq
                                  br_if $block_28
                                  call $_abort
                                end ;; $block_28
                              end ;; $if_108
                              local.get $4
                              local.get $6
                              i32.eq
                              if $if_110
                                i32.const 2500
                                i32.const 2500
                                i32.load
                                i32.const 1
                                local.get $1
                                i32.shl
                                i32.const -1
                                i32.xor
                                i32.and
                                i32.store
                                br $block_27
                              end ;; $if_110
                              local.get $0
                              local.get $6
                              i32.eq
                              if $if_111
                                local.get $6
                                i32.const 8
                                i32.add
                                local.set $20
                              else
                                block $block_29
                                  local.get $2
                                  local.get $6
                                  i32.gt_u
                                  if $if_112
                                    call $_abort
                                  end ;; $if_112
                                  local.get $6
                                  i32.const 8
                                  i32.add
                                  local.tee $0
                                  i32.load
                                  local.get $3
                                  i32.eq
                                  if $if_113
                                    local.get $0
                                    local.set $20
                                    br $block_29
                                  end ;; $if_113
                                  call $_abort
                                end ;; $block_29
                              end ;; $if_111
                              local.get $4
                              local.get $6
                              i32.store offset=12
                              local.get $20
                              local.get $4
                              i32.store
                            else
                              local.get $3
                              i32.load offset=24
                              local.set $8
                              local.get $3
                              i32.load offset=12
                              local.tee $0
                              local.get $3
                              i32.eq
                              if $if_114
                                block $block_30
                                  local.get $3
                                  i32.const 16
                                  i32.add
                                  local.tee $1
                                  i32.const 4
                                  i32.add
                                  local.tee $4
                                  i32.load
                                  local.tee $0
                                  if $if_115
                                    local.get $4
                                    local.set $1
                                  else
                                    local.get $1
                                    i32.load
                                    local.tee $0
                                    i32.eqz
                                    br_if $block_30
                                  end ;; $if_115
                                  loop $loop_8
                                    block $block_31
                                      local.get $0
                                      i32.const 20
                                      i32.add
                                      local.tee $4
                                      i32.load
                                      local.tee $6
                                      i32.eqz
                                      if $if_116
                                        local.get $0
                                        i32.const 16
                                        i32.add
                                        local.tee $4
                                        i32.load
                                        local.tee $6
                                        i32.eqz
                                        br_if $block_31
                                      end ;; $if_116
                                      local.get $4
                                      local.set $1
                                      local.get $6
                                      local.set $0
                                      br $loop_8
                                    end ;; $block_31
                                  end ;; $loop_8
                                  local.get $2
                                  local.get $1
                                  i32.gt_u
                                  if $if_117
                                    call $_abort
                                  else
                                    local.get $1
                                    i32.const 0
                                    i32.store
                                    local.get $0
                                    local.set $16
                                  end ;; $if_117
                                end ;; $block_30
                              else
                                local.get $2
                                local.get $3
                                i32.load offset=8
                                local.tee $1
                                i32.gt_u
                                if $if_118
                                  call $_abort
                                end ;; $if_118
                                local.get $3
                                local.get $1
                                i32.load offset=12
                                i32.ne
                                if $if_119
                                  call $_abort
                                end ;; $if_119
                                local.get $0
                                i32.load offset=8
                                local.get $3
                                i32.eq
                                if $if_120
                                  local.get $1
                                  local.get $0
                                  i32.store offset=12
                                  local.get $0
                                  local.get $1
                                  i32.store offset=8
                                  local.get $0
                                  local.set $16
                                else
                                  call $_abort
                                end ;; $if_120
                              end ;; $if_114
                              local.get $8
                              i32.eqz
                              br_if $block_27
                              local.get $3
                              i32.load offset=28
                              local.tee $1
                              i32.const 2
                              i32.shl
                              i32.const 2804
                              i32.add
                              local.tee $0
                              i32.load
                              local.get $3
                              i32.eq
                              if $if_121
                                block $block_32
                                  local.get $0
                                  local.get $16
                                  i32.store
                                  local.get $16
                                  br_if $block_32
                                  i32.const 2504
                                  i32.const 2504
                                  i32.load
                                  i32.const 1
                                  local.get $1
                                  i32.shl
                                  i32.const -1
                                  i32.xor
                                  i32.and
                                  i32.store
                                  br $block_27
                                end ;; $block_32
                              else
                                i32.const 2516
                                i32.load
                                local.get $8
                                i32.gt_u
                                if $if_122
                                  call $_abort
                                else
                                  local.get $8
                                  i32.const 16
                                  i32.add
                                  local.get $8
                                  i32.const 20
                                  i32.add
                                  local.get $8
                                  i32.load offset=16
                                  local.get $3
                                  i32.eq
                                  select
                                  local.get $16
                                  i32.store
                                  local.get $16
                                  i32.eqz
                                  br_if $block_27
                                end ;; $if_122
                              end ;; $if_121
                              i32.const 2516
                              i32.load
                              local.tee $0
                              local.get $16
                              i32.gt_u
                              if $if_123
                                call $_abort
                              end ;; $if_123
                              local.get $16
                              local.get $8
                              i32.store offset=24
                              local.get $3
                              i32.load offset=16
                              local.tee $1
                              if $if_124
                                local.get $0
                                local.get $1
                                i32.gt_u
                                if $if_125
                                  call $_abort
                                else
                                  local.get $16
                                  local.get $1
                                  i32.store offset=16
                                  local.get $1
                                  local.get $16
                                  i32.store offset=24
                                end ;; $if_125
                              end ;; $if_124
                              local.get $3
                              i32.load offset=20
                              local.tee $0
                              i32.eqz
                              br_if $block_27
                              i32.const 2516
                              i32.load
                              local.get $0
                              i32.gt_u
                              if $if_126
                                call $_abort
                              else
                                local.get $16
                                local.get $0
                                i32.store offset=20
                                local.get $0
                                local.get $16
                                i32.store offset=24
                              end ;; $if_126
                            end ;; $if_107
                          end ;; $block_27
                          local.get $3
                          local.get $10
                          i32.add
                          local.set $3
                          local.get $7
                          local.get $10
                          i32.add
                          local.set $7
                        end ;; $if_106
                        local.get $3
                        local.get $3
                        i32.load offset=4
                        i32.const -2
                        i32.and
                        i32.store offset=4
                        local.get $5
                        local.get $7
                        i32.const 1
                        i32.or
                        i32.store offset=4
                        local.get $5
                        local.get $7
                        i32.add
                        local.get $7
                        i32.store
                        local.get $7
                        i32.const 3
                        i32.shr_u
                        local.set $0
                        local.get $7
                        i32.const 256
                        i32.lt_u
                        if $if_127
                          local.get $0
                          i32.const 3
                          i32.shl
                          i32.const 2540
                          i32.add
                          local.set $2
                          i32.const 2500
                          i32.load
                          local.tee $1
                          i32.const 1
                          local.get $0
                          i32.shl
                          local.tee $0
                          i32.and
                          if $if_128
                            block $block_33
                              i32.const 2516
                              i32.load
                              local.get $2
                              i32.const 8
                              i32.add
                              local.tee $1
                              i32.load
                              local.tee $0
                              i32.le_u
                              if $if_129
                                local.get $1
                                local.set $21
                                local.get $0
                                local.set $18
                                br $block_33
                              end ;; $if_129
                              call $_abort
                            end ;; $block_33
                          else
                            i32.const 2500
                            local.get $0
                            local.get $1
                            i32.or
                            i32.store
                            local.get $2
                            i32.const 8
                            i32.add
                            local.set $21
                            local.get $2
                            local.set $18
                          end ;; $if_128
                          local.get $21
                          local.get $5
                          i32.store
                          local.get $18
                          local.get $5
                          i32.store offset=12
                          local.get $5
                          local.get $18
                          i32.store offset=8
                          local.get $5
                          local.get $2
                          i32.store offset=12
                          br $block_26
                        end ;; $if_127
                        local.get $7
                        i32.const 8
                        i32.shr_u
                        local.tee $0
                        if $if_130 (result i32)
                          local.get $7
                          i32.const 16777215
                          i32.gt_u
                          if $if_131 (result i32)
                            i32.const 31
                          else
                            local.get $0
                            local.get $0
                            i32.const 1048320
                            i32.add
                            i32.const 16
                            i32.shr_u
                            i32.const 8
                            i32.and
                            local.tee $2
                            i32.shl
                            local.tee $0
                            i32.const 520192
                            i32.add
                            i32.const 16
                            i32.shr_u
                            i32.const 4
                            i32.and
                            local.set $3
                            local.get $7
                            i32.const 14
                            local.get $0
                            local.get $3
                            i32.shl
                            local.tee $1
                            i32.const 245760
                            i32.add
                            i32.const 16
                            i32.shr_u
                            i32.const 2
                            i32.and
                            local.tee $0
                            local.get $2
                            local.get $3
                            i32.or
                            i32.or
                            i32.sub
                            local.get $1
                            local.get $0
                            i32.shl
                            i32.const 15
                            i32.shr_u
                            i32.add
                            local.tee $0
                            i32.const 7
                            i32.add
                            i32.shr_u
                            i32.const 1
                            i32.and
                            local.get $0
                            i32.const 1
                            i32.shl
                            i32.or
                          end ;; $if_131
                        else
                          i32.const 0
                        end ;; $if_130
                        local.tee $3
                        i32.const 2
                        i32.shl
                        i32.const 2804
                        i32.add
                        local.set $2
                        local.get $5
                        local.get $3
                        i32.store offset=28
                        local.get $5
                        i32.const 0
                        i32.store offset=20
                        local.get $5
                        i32.const 0
                        i32.store offset=16
                        i32.const 2504
                        i32.load
                        local.tee $1
                        i32.const 1
                        local.get $3
                        i32.shl
                        local.tee $0
                        i32.and
                        i32.eqz
                        if $if_132
                          i32.const 2504
                          local.get $0
                          local.get $1
                          i32.or
                          i32.store
                          local.get $2
                          local.get $5
                          i32.store
                          local.get $5
                          local.get $2
                          i32.store offset=24
                          local.get $5
                          local.get $5
                          i32.store offset=12
                          local.get $5
                          local.get $5
                          i32.store offset=8
                          br $block_26
                        end ;; $if_132
                        local.get $2
                        i32.load
                        local.tee $0
                        i32.load offset=4
                        i32.const -8
                        i32.and
                        local.get $7
                        i32.eq
                        if $if_133
                          local.get $0
                          local.set $9
                        else
                          block $block_34
                            local.get $7
                            i32.const 0
                            i32.const 25
                            local.get $3
                            i32.const 1
                            i32.shr_u
                            i32.sub
                            local.get $3
                            i32.const 31
                            i32.eq
                            select
                            i32.shl
                            local.set $3
                            loop $loop_9
                              local.get $0
                              i32.const 16
                              i32.add
                              local.get $3
                              i32.const 31
                              i32.shr_u
                              i32.const 2
                              i32.shl
                              i32.add
                              local.tee $2
                              i32.load
                              local.tee $1
                              if $if_134
                                local.get $3
                                i32.const 1
                                i32.shl
                                local.set $3
                                local.get $1
                                i32.load offset=4
                                i32.const -8
                                i32.and
                                local.get $7
                                i32.eq
                                if $if_135
                                  local.get $1
                                  local.set $9
                                  br $block_34
                                else
                                  local.get $1
                                  local.set $0
                                  br $loop_9
                                end ;; $if_135
                                unreachable
                              end ;; $if_134
                            end ;; $loop_9
                            i32.const 2516
                            i32.load
                            local.get $2
                            i32.gt_u
                            if $if_136
                              call $_abort
                            else
                              local.get $2
                              local.get $5
                              i32.store
                              local.get $5
                              local.get $0
                              i32.store offset=24
                              local.get $5
                              local.get $5
                              i32.store offset=12
                              local.get $5
                              local.get $5
                              i32.store offset=8
                              br $block_26
                            end ;; $if_136
                          end ;; $block_34
                        end ;; $if_133
                        i32.const 2516
                        i32.load
                        local.tee $0
                        local.get $9
                        i32.le_u
                        local.get $0
                        local.get $9
                        i32.load offset=8
                        local.tee $0
                        i32.le_u
                        i32.and
                        if $if_137
                          local.get $0
                          local.get $5
                          i32.store offset=12
                          local.get $9
                          local.get $5
                          i32.store offset=8
                          local.get $5
                          local.get $0
                          i32.store offset=8
                          local.get $5
                          local.get $9
                          i32.store offset=12
                          local.get $5
                          i32.const 0
                          i32.store offset=24
                        else
                          call $_abort
                        end ;; $if_137
                      end ;; $block_26
                    end ;; $if_104
                    local.get $17
                    global.set $22
                    local.get $12
                    i32.const 8
                    i32.add
                    return
                  end ;; $if_103
                end ;; $block_24
                i32.const 2948
                local.set $2
                loop $loop_10
                  block $block_35
                    local.get $2
                    i32.load
                    local.tee $1
                    local.get $6
                    i32.le_u
                    if $if_138
                      local.get $1
                      local.get $2
                      i32.load offset=4
                      i32.add
                      local.tee $7
                      local.get $6
                      i32.gt_u
                      br_if $block_35
                    end ;; $if_138
                    local.get $2
                    i32.load offset=8
                    local.set $2
                    br $loop_10
                  end ;; $block_35
                end ;; $loop_10
                i32.const 2524
                i32.const 0
                local.get $0
                i32.const 8
                i32.add
                local.tee $1
                i32.sub
                i32.const 7
                i32.and
                i32.const 0
                local.get $1
                i32.const 7
                i32.and
                select
                local.tee $1
                local.get $0
                i32.add
                local.tee $4
                i32.store
                i32.const 2512
                local.get $3
                i32.const -40
                i32.add
                local.tee $2
                local.get $1
                i32.sub
                local.tee $1
                i32.store
                local.get $4
                local.get $1
                i32.const 1
                i32.or
                i32.store offset=4
                local.get $0
                local.get $2
                i32.add
                i32.const 40
                i32.store offset=4
                i32.const 2528
                i32.const 2988
                i32.load
                i32.store
                local.get $6
                i32.const 0
                local.get $7
                i32.const -47
                i32.add
                local.tee $2
                i32.const 8
                i32.add
                local.tee $1
                i32.sub
                i32.const 7
                i32.and
                i32.const 0
                local.get $1
                i32.const 7
                i32.and
                select
                local.get $2
                i32.add
                local.tee $1
                local.get $1
                local.get $6
                i32.const 16
                i32.add
                i32.lt_u
                select
                local.tee $2
                i32.const 27
                i32.store offset=4
                local.get $2
                i32.const 2948
                i64.load align=4
                i64.store offset=8 align=4
                local.get $2
                i32.const 2956
                i64.load align=4
                i64.store offset=16 align=4
                i32.const 2948
                local.get $0
                i32.store
                i32.const 2952
                local.get $3
                i32.store
                i32.const 2960
                i32.const 0
                i32.store
                i32.const 2956
                local.get $2
                i32.const 8
                i32.add
                i32.store
                local.get $2
                i32.const 24
                i32.add
                local.set $0
                loop $loop_11
                  local.get $0
                  i32.const 4
                  i32.add
                  local.tee $1
                  i32.const 7
                  i32.store
                  local.get $0
                  i32.const 8
                  i32.add
                  local.get $7
                  i32.lt_u
                  if $if_139
                    local.get $1
                    local.set $0
                    br $loop_11
                  end ;; $if_139
                end ;; $loop_11
                local.get $2
                local.get $6
                i32.ne
                if $if_140
                  local.get $2
                  local.get $2
                  i32.load offset=4
                  i32.const -2
                  i32.and
                  i32.store offset=4
                  local.get $6
                  local.get $2
                  local.get $6
                  i32.sub
                  local.tee $4
                  i32.const 1
                  i32.or
                  i32.store offset=4
                  local.get $2
                  local.get $4
                  i32.store
                  local.get $4
                  i32.const 3
                  i32.shr_u
                  local.set $0
                  local.get $4
                  i32.const 256
                  i32.lt_u
                  if $if_141
                    local.get $0
                    i32.const 3
                    i32.shl
                    i32.const 2540
                    i32.add
                    local.set $2
                    i32.const 2500
                    i32.load
                    local.tee $1
                    i32.const 1
                    local.get $0
                    i32.shl
                    local.tee $0
                    i32.and
                    if $if_142
                      i32.const 2516
                      i32.load
                      local.get $2
                      i32.const 8
                      i32.add
                      local.tee $1
                      i32.load
                      local.tee $0
                      i32.gt_u
                      if $if_143
                        call $_abort
                      else
                        local.get $1
                        local.set $22
                        local.get $0
                        local.set $5
                      end ;; $if_143
                    else
                      i32.const 2500
                      local.get $0
                      local.get $1
                      i32.or
                      i32.store
                      local.get $2
                      i32.const 8
                      i32.add
                      local.set $22
                      local.get $2
                      local.set $5
                    end ;; $if_142
                    local.get $22
                    local.get $6
                    i32.store
                    local.get $5
                    local.get $6
                    i32.store offset=12
                    local.get $6
                    local.get $5
                    i32.store offset=8
                    local.get $6
                    local.get $2
                    i32.store offset=12
                    br $block_21
                  end ;; $if_141
                  local.get $4
                  i32.const 8
                  i32.shr_u
                  local.tee $0
                  if $if_144 (result i32)
                    local.get $4
                    i32.const 16777215
                    i32.gt_u
                    if $if_145 (result i32)
                      i32.const 31
                    else
                      local.get $0
                      local.get $0
                      i32.const 1048320
                      i32.add
                      i32.const 16
                      i32.shr_u
                      i32.const 8
                      i32.and
                      local.tee $2
                      i32.shl
                      local.tee $0
                      i32.const 520192
                      i32.add
                      i32.const 16
                      i32.shr_u
                      i32.const 4
                      i32.and
                      local.set $3
                      local.get $4
                      i32.const 14
                      local.get $0
                      local.get $3
                      i32.shl
                      local.tee $1
                      i32.const 245760
                      i32.add
                      i32.const 16
                      i32.shr_u
                      i32.const 2
                      i32.and
                      local.tee $0
                      local.get $2
                      local.get $3
                      i32.or
                      i32.or
                      i32.sub
                      local.get $1
                      local.get $0
                      i32.shl
                      i32.const 15
                      i32.shr_u
                      i32.add
                      local.tee $0
                      i32.const 7
                      i32.add
                      i32.shr_u
                      i32.const 1
                      i32.and
                      local.get $0
                      i32.const 1
                      i32.shl
                      i32.or
                    end ;; $if_145
                  else
                    i32.const 0
                  end ;; $if_144
                  local.tee $3
                  i32.const 2
                  i32.shl
                  i32.const 2804
                  i32.add
                  local.set $2
                  local.get $6
                  local.get $3
                  i32.store offset=28
                  local.get $6
                  i32.const 0
                  i32.store offset=20
                  local.get $6
                  i32.const 0
                  i32.store offset=16
                  i32.const 2504
                  i32.load
                  local.tee $1
                  i32.const 1
                  local.get $3
                  i32.shl
                  local.tee $0
                  i32.and
                  i32.eqz
                  if $if_146
                    i32.const 2504
                    local.get $0
                    local.get $1
                    i32.or
                    i32.store
                    local.get $2
                    local.get $6
                    i32.store
                    local.get $6
                    local.get $2
                    i32.store offset=24
                    local.get $6
                    local.get $6
                    i32.store offset=12
                    local.get $6
                    local.get $6
                    i32.store offset=8
                    br $block_21
                  end ;; $if_146
                  local.get $2
                  i32.load
                  local.tee $0
                  i32.load offset=4
                  i32.const -8
                  i32.and
                  local.get $4
                  i32.eq
                  if $if_147
                    local.get $0
                    local.set $10
                  else
                    block $block_36
                      local.get $4
                      i32.const 0
                      i32.const 25
                      local.get $3
                      i32.const 1
                      i32.shr_u
                      i32.sub
                      local.get $3
                      i32.const 31
                      i32.eq
                      select
                      i32.shl
                      local.set $2
                      loop $loop_12
                        local.get $0
                        i32.const 16
                        i32.add
                        local.get $2
                        i32.const 31
                        i32.shr_u
                        i32.const 2
                        i32.shl
                        i32.add
                        local.tee $3
                        i32.load
                        local.tee $1
                        if $if_148
                          local.get $2
                          i32.const 1
                          i32.shl
                          local.set $2
                          local.get $1
                          i32.load offset=4
                          i32.const -8
                          i32.and
                          local.get $4
                          i32.eq
                          if $if_149
                            local.get $1
                            local.set $10
                            br $block_36
                          else
                            local.get $1
                            local.set $0
                            br $loop_12
                          end ;; $if_149
                          unreachable
                        end ;; $if_148
                      end ;; $loop_12
                      i32.const 2516
                      i32.load
                      local.get $3
                      i32.gt_u
                      if $if_150
                        call $_abort
                      else
                        local.get $3
                        local.get $6
                        i32.store
                        local.get $6
                        local.get $0
                        i32.store offset=24
                        local.get $6
                        local.get $6
                        i32.store offset=12
                        local.get $6
                        local.get $6
                        i32.store offset=8
                        br $block_21
                      end ;; $if_150
                    end ;; $block_36
                  end ;; $if_147
                  i32.const 2516
                  i32.load
                  local.tee $0
                  local.get $10
                  i32.le_u
                  local.get $0
                  local.get $10
                  i32.load offset=8
                  local.tee $0
                  i32.le_u
                  i32.and
                  if $if_151
                    local.get $0
                    local.get $6
                    i32.store offset=12
                    local.get $10
                    local.get $6
                    i32.store offset=8
                    local.get $6
                    local.get $0
                    i32.store offset=8
                    local.get $6
                    local.get $10
                    i32.store offset=12
                    local.get $6
                    i32.const 0
                    i32.store offset=24
                  else
                    call $_abort
                  end ;; $if_151
                end ;; $if_140
              end ;; $block_21
            else
              i32.const 2516
              i32.load
              local.tee $1
              i32.eqz
              local.get $0
              local.get $1
              i32.lt_u
              i32.or
              if $if_152
                i32.const 2516
                local.get $0
                i32.store
              end ;; $if_152
              i32.const 2948
              local.get $0
              i32.store
              i32.const 2952
              local.get $3
              i32.store
              i32.const 2960
              i32.const 0
              i32.store
              i32.const 2536
              i32.const 2972
              i32.load
              i32.store
              i32.const 2532
              i32.const -1
              i32.store
              i32.const 2552
              i32.const 2540
              i32.store
              i32.const 2548
              i32.const 2540
              i32.store
              i32.const 2560
              i32.const 2548
              i32.store
              i32.const 2556
              i32.const 2548
              i32.store
              i32.const 2568
              i32.const 2556
              i32.store
              i32.const 2564
              i32.const 2556
              i32.store
              i32.const 2576
              i32.const 2564
              i32.store
              i32.const 2572
              i32.const 2564
              i32.store
              i32.const 2584
              i32.const 2572
              i32.store
              i32.const 2580
              i32.const 2572
              i32.store
              i32.const 2592
              i32.const 2580
              i32.store
              i32.const 2588
              i32.const 2580
              i32.store
              i32.const 2600
              i32.const 2588
              i32.store
              i32.const 2596
              i32.const 2588
              i32.store
              i32.const 2608
              i32.const 2596
              i32.store
              i32.const 2604
              i32.const 2596
              i32.store
              i32.const 2616
              i32.const 2604
              i32.store
              i32.const 2612
              i32.const 2604
              i32.store
              i32.const 2624
              i32.const 2612
              i32.store
              i32.const 2620
              i32.const 2612
              i32.store
              i32.const 2632
              i32.const 2620
              i32.store
              i32.const 2628
              i32.const 2620
              i32.store
              i32.const 2640
              i32.const 2628
              i32.store
              i32.const 2636
              i32.const 2628
              i32.store
              i32.const 2648
              i32.const 2636
              i32.store
              i32.const 2644
              i32.const 2636
              i32.store
              i32.const 2656
              i32.const 2644
              i32.store
              i32.const 2652
              i32.const 2644
              i32.store
              i32.const 2664
              i32.const 2652
              i32.store
              i32.const 2660
              i32.const 2652
              i32.store
              i32.const 2672
              i32.const 2660
              i32.store
              i32.const 2668
              i32.const 2660
              i32.store
              i32.const 2680
              i32.const 2668
              i32.store
              i32.const 2676
              i32.const 2668
              i32.store
              i32.const 2688
              i32.const 2676
              i32.store
              i32.const 2684
              i32.const 2676
              i32.store
              i32.const 2696
              i32.const 2684
              i32.store
              i32.const 2692
              i32.const 2684
              i32.store
              i32.const 2704
              i32.const 2692
              i32.store
              i32.const 2700
              i32.const 2692
              i32.store
              i32.const 2712
              i32.const 2700
              i32.store
              i32.const 2708
              i32.const 2700
              i32.store
              i32.const 2720
              i32.const 2708
              i32.store
              i32.const 2716
              i32.const 2708
              i32.store
              i32.const 2728
              i32.const 2716
              i32.store
              i32.const 2724
              i32.const 2716
              i32.store
              i32.const 2736
              i32.const 2724
              i32.store
              i32.const 2732
              i32.const 2724
              i32.store
              i32.const 2744
              i32.const 2732
              i32.store
              i32.const 2740
              i32.const 2732
              i32.store
              i32.const 2752
              i32.const 2740
              i32.store
              i32.const 2748
              i32.const 2740
              i32.store
              i32.const 2760
              i32.const 2748
              i32.store
              i32.const 2756
              i32.const 2748
              i32.store
              i32.const 2768
              i32.const 2756
              i32.store
              i32.const 2764
              i32.const 2756
              i32.store
              i32.const 2776
              i32.const 2764
              i32.store
              i32.const 2772
              i32.const 2764
              i32.store
              i32.const 2784
              i32.const 2772
              i32.store
              i32.const 2780
              i32.const 2772
              i32.store
              i32.const 2792
              i32.const 2780
              i32.store
              i32.const 2788
              i32.const 2780
              i32.store
              i32.const 2800
              i32.const 2788
              i32.store
              i32.const 2796
              i32.const 2788
              i32.store
              i32.const 2524
              i32.const 0
              local.get $0
              i32.const 8
              i32.add
              local.tee $1
              i32.sub
              i32.const 7
              i32.and
              i32.const 0
              local.get $1
              i32.const 7
              i32.and
              select
              local.tee $1
              local.get $0
              i32.add
              local.tee $4
              i32.store
              i32.const 2512
              local.get $3
              i32.const -40
              i32.add
              local.tee $2
              local.get $1
              i32.sub
              local.tee $1
              i32.store
              local.get $4
              local.get $1
              i32.const 1
              i32.or
              i32.store offset=4
              local.get $0
              local.get $2
              i32.add
              i32.const 40
              i32.store offset=4
              i32.const 2528
              i32.const 2988
              i32.load
              i32.store
            end ;; $if_99
            i32.const 2512
            i32.load
            local.tee $0
            local.get $11
            i32.gt_u
            if $if_153
              i32.const 2512
              local.get $0
              local.get $11
              i32.sub
              local.tee $3
              i32.store
              br $block_14
            end ;; $if_153
          end ;; $block_15
          i32.const 2496
          i32.const 12
          i32.store
          br $block_12
        end ;; $block_14
        i32.const 2524
        i32.const 2524
        i32.load
        local.tee $0
        local.get $11
        i32.add
        local.tee $1
        i32.store
        local.get $1
        local.get $3
        i32.const 1
        i32.or
        i32.store offset=4
        local.get $0
        local.get $11
        i32.const 3
        i32.or
        i32.store offset=4
      end ;; $block_13
      local.get $17
      global.set $22
      local.get $0
      i32.const 8
      i32.add
      return
    end ;; $block_12
    local.get $17
    global.set $22
    i32.const 0
    )
  
  (func $_free (type $7)
    (param $0 i32)
    (local $1 i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    (local $7 i32)
    (local $8 i32)
    (local $9 i32)
    (local $10 i32)
    (local $11 i32)
    (local $12 i32)
    (local $13 i32)
    (local $14 i32)
    (local $15 i32)
    (local $16 i32)
    (local $17 i32)
    local.get $0
    i32.eqz
    if $if
      return
    end ;; $if
    local.get $0
    i32.const -8
    i32.add
    local.tee $5
    i32.const 2516
    i32.load
    local.tee $11
    i32.lt_u
    if $if_0
      call $_abort
    end ;; $if_0
    local.get $0
    i32.const -4
    i32.add
    i32.load
    local.tee $0
    i32.const 3
    i32.and
    local.tee $12
    i32.const 1
    i32.eq
    if $if_1
      call $_abort
    end ;; $if_1
    local.get $5
    local.get $0
    i32.const -8
    i32.and
    local.tee $2
    i32.add
    local.set $7
    local.get $0
    i32.const 1
    i32.and
    if $if_2
      local.get $5
      local.tee $4
      local.set $3
      local.get $2
      local.set $1
    else
      block $block
        local.get $5
        i32.load
        local.set $10
        local.get $12
        i32.eqz
        if $if_3
          return
        end ;; $if_3
        local.get $5
        local.get $10
        i32.sub
        local.tee $0
        local.get $11
        i32.lt_u
        if $if_4
          call $_abort
        end ;; $if_4
        local.get $2
        local.get $10
        i32.add
        local.set $5
        i32.const 2520
        i32.load
        local.get $0
        i32.eq
        if $if_5
          local.get $7
          i32.load offset=4
          local.tee $4
          i32.const 3
          i32.and
          i32.const 3
          i32.ne
          if $if_6
            local.get $0
            local.tee $4
            local.set $3
            local.get $5
            local.set $1
            br $block
          end ;; $if_6
          i32.const 2508
          local.get $5
          i32.store
          local.get $7
          local.get $4
          i32.const -2
          i32.and
          i32.store offset=4
          local.get $0
          local.get $5
          i32.const 1
          i32.or
          i32.store offset=4
          local.get $0
          local.get $5
          i32.add
          local.get $5
          i32.store
          return
        end ;; $if_5
        local.get $10
        i32.const 3
        i32.shr_u
        local.set $2
        local.get $10
        i32.const 256
        i32.lt_u
        if $if_7
          local.get $0
          i32.load offset=12
          local.set $1
          local.get $0
          i32.load offset=8
          local.tee $3
          local.get $2
          i32.const 3
          i32.shl
          i32.const 2540
          i32.add
          local.tee $4
          i32.ne
          if $if_8
            local.get $11
            local.get $3
            i32.gt_u
            if $if_9
              call $_abort
            end ;; $if_9
            local.get $0
            local.get $3
            i32.load offset=12
            i32.ne
            if $if_10
              call $_abort
            end ;; $if_10
          end ;; $if_8
          local.get $1
          local.get $3
          i32.eq
          if $if_11
            i32.const 2500
            i32.const 2500
            i32.load
            i32.const 1
            local.get $2
            i32.shl
            i32.const -1
            i32.xor
            i32.and
            i32.store
            local.get $0
            local.tee $4
            local.set $3
            local.get $5
            local.set $1
            br $block
          end ;; $if_11
          local.get $1
          local.get $4
          i32.eq
          if $if_12
            local.get $1
            i32.const 8
            i32.add
            local.set $6
          else
            local.get $11
            local.get $1
            i32.gt_u
            if $if_13
              call $_abort
            end ;; $if_13
            local.get $1
            i32.const 8
            i32.add
            local.tee $4
            i32.load
            local.get $0
            i32.eq
            if $if_14
              local.get $4
              local.set $6
            else
              call $_abort
            end ;; $if_14
          end ;; $if_12
          local.get $3
          local.get $1
          i32.store offset=12
          local.get $6
          local.get $3
          i32.store
          local.get $0
          local.tee $4
          local.set $3
          local.get $5
          local.set $1
          br $block
        end ;; $if_7
        local.get $0
        i32.load offset=24
        local.set $13
        local.get $0
        i32.load offset=12
        local.tee $2
        local.get $0
        i32.eq
        if $if_15
          block $block_0
            local.get $0
            i32.const 16
            i32.add
            local.tee $6
            i32.const 4
            i32.add
            local.tee $10
            i32.load
            local.tee $2
            if $if_16
              local.get $10
              local.set $6
            else
              local.get $6
              i32.load
              local.tee $2
              i32.eqz
              br_if $block_0
            end ;; $if_16
            loop $loop
              block $block_1
                local.get $2
                i32.const 20
                i32.add
                local.tee $10
                i32.load
                local.tee $12
                i32.eqz
                if $if_17
                  local.get $2
                  i32.const 16
                  i32.add
                  local.tee $10
                  i32.load
                  local.tee $12
                  i32.eqz
                  br_if $block_1
                end ;; $if_17
                local.get $10
                local.set $6
                local.get $12
                local.set $2
                br $loop
              end ;; $block_1
            end ;; $loop
            local.get $11
            local.get $6
            i32.gt_u
            if $if_18
              call $_abort
            else
              local.get $6
              i32.const 0
              i32.store
              local.get $2
              local.set $8
            end ;; $if_18
          end ;; $block_0
        else
          local.get $11
          local.get $0
          i32.load offset=8
          local.tee $6
          i32.gt_u
          if $if_19
            call $_abort
          end ;; $if_19
          local.get $0
          local.get $6
          i32.load offset=12
          i32.ne
          if $if_20
            call $_abort
          end ;; $if_20
          local.get $2
          i32.load offset=8
          local.get $0
          i32.eq
          if $if_21
            local.get $6
            local.get $2
            i32.store offset=12
            local.get $2
            local.get $6
            i32.store offset=8
            local.get $2
            local.set $8
          else
            call $_abort
          end ;; $if_21
        end ;; $if_15
        local.get $13
        if $if_22
          local.get $0
          i32.load offset=28
          local.tee $2
          i32.const 2
          i32.shl
          i32.const 2804
          i32.add
          local.tee $6
          i32.load
          local.get $0
          i32.eq
          if $if_23
            local.get $6
            local.get $8
            i32.store
            local.get $8
            i32.eqz
            if $if_24
              i32.const 2504
              i32.const 2504
              i32.load
              i32.const 1
              local.get $2
              i32.shl
              i32.const -1
              i32.xor
              i32.and
              i32.store
              local.get $0
              local.tee $4
              local.set $3
              local.get $5
              local.set $1
              br $block
            end ;; $if_24
          else
            i32.const 2516
            i32.load
            local.get $13
            i32.gt_u
            if $if_25
              call $_abort
            else
              local.get $13
              i32.const 16
              i32.add
              local.tee $2
              local.get $13
              i32.const 20
              i32.add
              local.get $2
              i32.load
              local.get $0
              i32.eq
              select
              local.get $8
              i32.store
              local.get $8
              i32.eqz
              if $if_26
                local.get $0
                local.tee $4
                local.set $3
                local.get $5
                local.set $1
                br $block
              end ;; $if_26
            end ;; $if_25
          end ;; $if_23
          i32.const 2516
          i32.load
          local.tee $6
          local.get $8
          i32.gt_u
          if $if_27
            call $_abort
          end ;; $if_27
          local.get $8
          local.get $13
          i32.store offset=24
          local.get $0
          i32.load offset=16
          local.tee $2
          if $if_28
            local.get $6
            local.get $2
            i32.gt_u
            if $if_29
              call $_abort
            else
              local.get $8
              local.get $2
              i32.store offset=16
              local.get $2
              local.get $8
              i32.store offset=24
            end ;; $if_29
          end ;; $if_28
          local.get $0
          i32.load offset=20
          local.tee $2
          if $if_30
            i32.const 2516
            i32.load
            local.get $2
            i32.gt_u
            if $if_31
              call $_abort
            else
              local.get $8
              local.get $2
              i32.store offset=20
              local.get $2
              local.get $8
              i32.store offset=24
              local.get $0
              local.tee $4
              local.set $3
              local.get $5
              local.set $1
            end ;; $if_31
          else
            local.get $0
            local.tee $4
            local.set $3
            local.get $5
            local.set $1
          end ;; $if_30
        else
          local.get $0
          local.tee $4
          local.set $3
          local.get $5
          local.set $1
        end ;; $if_22
      end ;; $block
    end ;; $if_2
    local.get $4
    local.get $7
    i32.ge_u
    if $if_32
      call $_abort
    end ;; $if_32
    local.get $7
    i32.load offset=4
    local.tee $0
    i32.const 1
    i32.and
    i32.eqz
    if $if_33
      call $_abort
    end ;; $if_33
    local.get $0
    i32.const 2
    i32.and
    if $if_34
      local.get $7
      local.get $0
      i32.const -2
      i32.and
      i32.store offset=4
      local.get $3
      local.get $1
      i32.const 1
      i32.or
      i32.store offset=4
      local.get $1
      local.get $4
      i32.add
      local.get $1
      i32.store
    else
      i32.const 2524
      i32.load
      local.get $7
      i32.eq
      if $if_35
        i32.const 2512
        i32.const 2512
        i32.load
        local.get $1
        i32.add
        local.tee $0
        i32.store
        i32.const 2524
        local.get $3
        i32.store
        local.get $3
        local.get $0
        i32.const 1
        i32.or
        i32.store offset=4
        local.get $3
        i32.const 2520
        i32.load
        i32.ne
        if $if_36
          return
        end ;; $if_36
        i32.const 2520
        i32.const 0
        i32.store
        i32.const 2508
        i32.const 0
        i32.store
        return
      end ;; $if_35
      i32.const 2520
      i32.load
      local.get $7
      i32.eq
      if $if_37
        i32.const 2508
        i32.const 2508
        i32.load
        local.get $1
        i32.add
        local.tee $0
        i32.store
        i32.const 2520
        local.get $4
        i32.store
        local.get $3
        local.get $0
        i32.const 1
        i32.or
        i32.store offset=4
        local.get $0
        local.get $4
        i32.add
        local.get $0
        i32.store
        return
      end ;; $if_37
      local.get $0
      i32.const -8
      i32.and
      local.get $1
      i32.add
      local.set $5
      local.get $0
      i32.const 3
      i32.shr_u
      local.set $6
      block $block_2
        local.get $0
        i32.const 256
        i32.lt_u
        if $if_38
          local.get $7
          i32.load offset=12
          local.set $1
          local.get $7
          i32.load offset=8
          local.tee $2
          local.get $6
          i32.const 3
          i32.shl
          i32.const 2540
          i32.add
          local.tee $0
          i32.ne
          if $if_39
            i32.const 2516
            i32.load
            local.get $2
            i32.gt_u
            if $if_40
              call $_abort
            end ;; $if_40
            local.get $7
            local.get $2
            i32.load offset=12
            i32.ne
            if $if_41
              call $_abort
            end ;; $if_41
          end ;; $if_39
          local.get $1
          local.get $2
          i32.eq
          if $if_42
            i32.const 2500
            i32.const 2500
            i32.load
            i32.const 1
            local.get $6
            i32.shl
            i32.const -1
            i32.xor
            i32.and
            i32.store
            br $block_2
          end ;; $if_42
          local.get $0
          local.get $1
          i32.eq
          if $if_43
            local.get $1
            i32.const 8
            i32.add
            local.set $16
          else
            i32.const 2516
            i32.load
            local.get $1
            i32.gt_u
            if $if_44
              call $_abort
            end ;; $if_44
            local.get $1
            i32.const 8
            i32.add
            local.tee $0
            i32.load
            local.get $7
            i32.eq
            if $if_45
              local.get $0
              local.set $16
            else
              call $_abort
            end ;; $if_45
          end ;; $if_43
          local.get $2
          local.get $1
          i32.store offset=12
          local.get $16
          local.get $2
          i32.store
        else
          local.get $7
          i32.load offset=24
          local.set $8
          local.get $7
          i32.load offset=12
          local.tee $0
          local.get $7
          i32.eq
          if $if_46
            block $block_3
              local.get $7
              i32.const 16
              i32.add
              local.tee $1
              i32.const 4
              i32.add
              local.tee $2
              i32.load
              local.tee $0
              if $if_47
                local.get $2
                local.set $1
              else
                local.get $1
                i32.load
                local.tee $0
                i32.eqz
                br_if $block_3
              end ;; $if_47
              loop $loop_0
                block $block_4
                  local.get $0
                  i32.const 20
                  i32.add
                  local.tee $2
                  i32.load
                  local.tee $6
                  i32.eqz
                  if $if_48
                    local.get $0
                    i32.const 16
                    i32.add
                    local.tee $2
                    i32.load
                    local.tee $6
                    i32.eqz
                    br_if $block_4
                  end ;; $if_48
                  local.get $2
                  local.set $1
                  local.get $6
                  local.set $0
                  br $loop_0
                end ;; $block_4
              end ;; $loop_0
              i32.const 2516
              i32.load
              local.get $1
              i32.gt_u
              if $if_49
                call $_abort
              else
                local.get $1
                i32.const 0
                i32.store
                local.get $0
                local.set $9
              end ;; $if_49
            end ;; $block_3
          else
            i32.const 2516
            i32.load
            local.get $7
            i32.load offset=8
            local.tee $1
            i32.gt_u
            if $if_50
              call $_abort
            end ;; $if_50
            local.get $7
            local.get $1
            i32.load offset=12
            i32.ne
            if $if_51
              call $_abort
            end ;; $if_51
            local.get $0
            i32.load offset=8
            local.get $7
            i32.eq
            if $if_52
              local.get $1
              local.get $0
              i32.store offset=12
              local.get $0
              local.get $1
              i32.store offset=8
              local.get $0
              local.set $9
            else
              call $_abort
            end ;; $if_52
          end ;; $if_46
          local.get $8
          if $if_53
            local.get $7
            i32.load offset=28
            local.tee $0
            i32.const 2
            i32.shl
            i32.const 2804
            i32.add
            local.tee $1
            i32.load
            local.get $7
            i32.eq
            if $if_54
              local.get $1
              local.get $9
              i32.store
              local.get $9
              i32.eqz
              if $if_55
                i32.const 2504
                i32.const 2504
                i32.load
                i32.const 1
                local.get $0
                i32.shl
                i32.const -1
                i32.xor
                i32.and
                i32.store
                br $block_2
              end ;; $if_55
            else
              i32.const 2516
              i32.load
              local.get $8
              i32.gt_u
              if $if_56
                call $_abort
              else
                local.get $8
                i32.const 16
                i32.add
                local.tee $0
                local.get $8
                i32.const 20
                i32.add
                local.get $0
                i32.load
                local.get $7
                i32.eq
                select
                local.get $9
                i32.store
                local.get $9
                i32.eqz
                br_if $block_2
              end ;; $if_56
            end ;; $if_54
            i32.const 2516
            i32.load
            local.tee $1
            local.get $9
            i32.gt_u
            if $if_57
              call $_abort
            end ;; $if_57
            local.get $9
            local.get $8
            i32.store offset=24
            local.get $7
            i32.load offset=16
            local.tee $0
            if $if_58
              local.get $1
              local.get $0
              i32.gt_u
              if $if_59
                call $_abort
              else
                local.get $9
                local.get $0
                i32.store offset=16
                local.get $0
                local.get $9
                i32.store offset=24
              end ;; $if_59
            end ;; $if_58
            local.get $7
            i32.load offset=20
            local.tee $0
            if $if_60
              i32.const 2516
              i32.load
              local.get $0
              i32.gt_u
              if $if_61
                call $_abort
              else
                local.get $9
                local.get $0
                i32.store offset=20
                local.get $0
                local.get $9
                i32.store offset=24
              end ;; $if_61
            end ;; $if_60
          end ;; $if_53
        end ;; $if_38
      end ;; $block_2
      local.get $3
      local.get $5
      i32.const 1
      i32.or
      i32.store offset=4
      local.get $4
      local.get $5
      i32.add
      local.get $5
      i32.store
      i32.const 2520
      i32.load
      local.get $3
      i32.eq
      if $if_62 (result i32)
        i32.const 2508
        local.get $5
        i32.store
        return
      else
        local.get $5
      end ;; $if_62
      local.set $1
    end ;; $if_34
    local.get $1
    i32.const 3
    i32.shr_u
    local.set $4
    local.get $1
    i32.const 256
    i32.lt_u
    if $if_63
      local.get $4
      i32.const 3
      i32.shl
      i32.const 2540
      i32.add
      local.set $0
      i32.const 2500
      i32.load
      local.tee $1
      i32.const 1
      local.get $4
      i32.shl
      local.tee $4
      i32.and
      if $if_64
        i32.const 2516
        i32.load
        local.get $0
        i32.const 8
        i32.add
        local.tee $4
        i32.load
        local.tee $1
        i32.gt_u
        if $if_65
          call $_abort
        else
          local.get $4
          local.set $17
          local.get $1
          local.set $15
        end ;; $if_65
      else
        i32.const 2500
        local.get $1
        local.get $4
        i32.or
        i32.store
        local.get $0
        i32.const 8
        i32.add
        local.set $17
        local.get $0
        local.set $15
      end ;; $if_64
      local.get $17
      local.get $3
      i32.store
      local.get $15
      local.get $3
      i32.store offset=12
      local.get $3
      local.get $15
      i32.store offset=8
      local.get $3
      local.get $0
      i32.store offset=12
      return
    end ;; $if_63
    local.get $1
    i32.const 8
    i32.shr_u
    local.tee $0
    if $if_66 (result i32)
      local.get $1
      i32.const 16777215
      i32.gt_u
      if $if_67 (result i32)
        i32.const 31
      else
        local.get $0
        local.get $0
        i32.const 1048320
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 8
        i32.and
        local.tee $5
        i32.shl
        local.tee $4
        i32.const 520192
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 4
        i32.and
        local.set $0
        local.get $4
        local.get $0
        i32.shl
        local.tee $2
        i32.const 245760
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 2
        i32.and
        local.set $4
        local.get $1
        i32.const 14
        local.get $0
        local.get $5
        i32.or
        local.get $4
        i32.or
        i32.sub
        local.get $2
        local.get $4
        i32.shl
        i32.const 15
        i32.shr_u
        i32.add
        local.tee $0
        i32.const 7
        i32.add
        i32.shr_u
        i32.const 1
        i32.and
        local.get $0
        i32.const 1
        i32.shl
        i32.or
      end ;; $if_67
    else
      i32.const 0
    end ;; $if_66
    local.tee $4
    i32.const 2
    i32.shl
    i32.const 2804
    i32.add
    local.set $0
    local.get $3
    local.get $4
    i32.store offset=28
    local.get $3
    i32.const 0
    i32.store offset=20
    local.get $3
    i32.const 0
    i32.store offset=16
    i32.const 2504
    i32.load
    local.tee $5
    i32.const 1
    local.get $4
    i32.shl
    local.tee $2
    i32.and
    if $if_68
      block $block_5
        local.get $0
        i32.load
        local.tee $0
        i32.load offset=4
        i32.const -8
        i32.and
        local.get $1
        i32.eq
        if $if_69
          local.get $0
          local.set $14
        else
          block $block_6
            local.get $1
            i32.const 0
            i32.const 25
            local.get $4
            i32.const 1
            i32.shr_u
            i32.sub
            local.get $4
            i32.const 31
            i32.eq
            select
            i32.shl
            local.set $5
            loop $loop_1
              local.get $0
              i32.const 16
              i32.add
              local.get $5
              i32.const 31
              i32.shr_u
              i32.const 2
              i32.shl
              i32.add
              local.tee $2
              i32.load
              local.tee $4
              if $if_70
                local.get $5
                i32.const 1
                i32.shl
                local.set $5
                local.get $4
                i32.load offset=4
                i32.const -8
                i32.and
                local.get $1
                i32.eq
                if $if_71
                  local.get $4
                  local.set $14
                  br $block_6
                else
                  local.get $4
                  local.set $0
                  br $loop_1
                end ;; $if_71
                unreachable
              end ;; $if_70
            end ;; $loop_1
            i32.const 2516
            i32.load
            local.get $2
            i32.gt_u
            if $if_72
              call $_abort
            else
              local.get $2
              local.get $3
              i32.store
              local.get $3
              local.get $0
              i32.store offset=24
              local.get $3
              local.get $3
              i32.store offset=12
              local.get $3
              local.get $3
              i32.store offset=8
              br $block_5
            end ;; $if_72
          end ;; $block_6
        end ;; $if_69
        i32.const 2516
        i32.load
        local.tee $0
        local.get $14
        i32.le_u
        local.get $0
        local.get $14
        i32.load offset=8
        local.tee $0
        i32.le_u
        i32.and
        if $if_73
          local.get $0
          local.get $3
          i32.store offset=12
          local.get $14
          local.get $3
          i32.store offset=8
          local.get $3
          local.get $0
          i32.store offset=8
          local.get $3
          local.get $14
          i32.store offset=12
          local.get $3
          i32.const 0
          i32.store offset=24
        else
          call $_abort
        end ;; $if_73
      end ;; $block_5
    else
      i32.const 2504
      local.get $2
      local.get $5
      i32.or
      i32.store
      local.get $0
      local.get $3
      i32.store
      local.get $3
      local.get $0
      i32.store offset=24
      local.get $3
      local.get $3
      i32.store offset=12
      local.get $3
      local.get $3
      i32.store offset=8
    end ;; $if_68
    i32.const 2532
    i32.const 2532
    i32.load
    i32.const -1
    i32.add
    local.tee $0
    i32.store
    local.get $0
    if $if_74
      return
    end ;; $if_74
    i32.const 2956
    local.set $0
    loop $loop_2
      local.get $0
      i32.load
      local.tee $4
      i32.const 8
      i32.add
      local.set $0
      local.get $4
      br_if $loop_2
    end ;; $loop_2
    i32.const 2532
    i32.const -1
    i32.store
    )
  
  (func $__ZSt18uncaught_exceptionv (type $8)
    (result i32)
    call $___cxa_uncaught_exceptions
    i32.const 0
    i32.gt_s
    )
  
  (func $__ZNSt3__211char_traitsIcE4copyEPcPKcm (type $10)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    local.get $2
    if $if
      local.get $0
      local.get $1
      local.get $2
      call $_memcpy
      drop
    end ;; $if
    )
  
  (func $__Znwm (type $9)
    (param $0 i32)
    (result i32)
    local.get $0
    i32.const 1
    local.get $0
    select
    local.set $0
    local.get $0
    call $_malloc
    local.tee $0
    if $if (result i32)
      local.get $0
    else
      i32.const 0
    end ;; $if
    )
  
  (func $__ZNSt3__211char_traitsIcE4moveEPcPKcm (type $10)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    local.get $2
    if $if
      local.get $0
      local.get $1
      local.get $2
      call $_memmove
      drop
    end ;; $if
    )
  
  (func $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE21__grow_by_and_replaceEmmmmmmPKc (type $6)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (param $4 i32)
    (local $5 i32)
    (local $6 i32)
    (local $7 i32)
    global.get $22
    local.set $5
    global.get $22
    i32.const 16
    i32.add
    global.set $22
    i32.const -18
    local.get $1
    i32.sub
    local.get $2
    i32.lt_u
    if $if
      call $_abort
    end ;; $if
    local.get $0
    i32.load8_s offset=11
    i32.const 0
    i32.lt_s
    if $if_0 (result i32)
      local.get $0
      i32.load
    else
      local.get $0
    end ;; $if_0
    local.set $7
    local.get $1
    i32.const 2147483623
    i32.lt_u
    if $if_1 (result i32)
      i32.const 11
      local.get $1
      i32.const 1
      i32.shl
      local.tee $6
      local.get $1
      local.get $2
      i32.add
      local.tee $2
      local.get $2
      local.get $6
      i32.lt_u
      select
      local.tee $2
      i32.const 16
      i32.add
      i32.const -16
      i32.and
      local.get $2
      i32.const 11
      i32.lt_u
      select
    else
      i32.const -17
    end ;; $if_1
    local.tee $6
    call $__Znwm
    local.set $2
    local.get $4
    if $if_2
      local.get $2
      i32.const 2093
      local.get $4
      call $__ZNSt3__211char_traitsIcE4copyEPcPKcm
    end ;; $if_2
    local.get $3
    if $if_3
      local.get $2
      local.get $4
      i32.add
      local.get $7
      local.get $3
      call $__ZNSt3__211char_traitsIcE4copyEPcPKcm
    end ;; $if_3
    local.get $1
    i32.const 10
    i32.ne
    if $if_4
      local.get $7
      call $_free
    end ;; $if_4
    local.get $0
    local.get $2
    i32.store
    local.get $0
    local.get $6
    i32.const -2147483648
    i32.or
    i32.store offset=8
    local.get $0
    local.get $3
    local.get $4
    i32.add
    local.tee $0
    i32.store offset=4
    local.get $5
    i32.const 0
    i32.store8
    local.get $0
    local.get $2
    i32.add
    local.get $5
    i32.load8_s
    i32.store8
    local.get $5
    global.set $22
    )
  
  (func $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6resizeEmc (type $2)
    (param $0 i32)
    (param $1 i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    global.get $22
    local.set $2
    global.get $22
    i32.const 16
    i32.add
    global.set $22
    local.get $0
    i32.load8_s offset=11
    local.tee $3
    i32.const 0
    i32.lt_s
    local.tee $4
    if $if (result i32)
      local.get $0
      i32.load offset=4
    else
      local.get $3
      i32.const 255
      i32.and
    end ;; $if
    local.tee $3
    local.get $1
    i32.lt_u
    if $if_0
      local.get $0
      local.get $1
      local.get $3
      i32.sub
      call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6appendEmc
    else
      local.get $4
      if $if_1
        local.get $1
        local.get $0
        i32.load
        i32.add
        local.set $3
        local.get $2
        i32.const 0
        i32.store8
        local.get $3
        local.get $2
        i32.load8_s
        i32.store8
        local.get $0
        local.get $1
        i32.store offset=4
      else
        local.get $2
        i32.const 0
        i32.store8
        local.get $0
        local.get $1
        i32.add
        local.get $2
        i32.load8_s
        i32.store8
        local.get $0
        local.get $1
        i32.store8 offset=11
      end ;; $if_1
    end ;; $if_0
    local.get $2
    global.set $22
    )
  
  (func $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6appendEmc (type $2)
    (param $0 i32)
    (param $1 i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    global.get $22
    local.set $5
    global.get $22
    i32.const 16
    i32.add
    global.set $22
    local.get $1
    if $if
      local.get $0
      i32.load8_s offset=11
      local.tee $3
      i32.const 0
      i32.lt_s
      if $if_0 (result i32)
        local.get $0
        i32.load offset=4
        local.set $2
        local.get $0
        i32.load offset=8
        i32.const 2147483647
        i32.and
        i32.const -1
        i32.add
      else
        local.get $3
        i32.const 255
        i32.and
        local.set $2
        i32.const 10
      end ;; $if_0
      local.tee $4
      local.get $2
      i32.sub
      local.get $1
      i32.lt_u
      if $if_1 (result i32)
        local.get $0
        local.get $4
        local.get $1
        local.get $2
        i32.add
        local.get $4
        i32.sub
        local.get $2
        local.get $2
        call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE9__grow_byEmmmmmm
        local.get $0
        i32.load8_s offset=11
      else
        local.get $3
      end ;; $if_1
      i32.const 0
      i32.lt_s
      if $if_2 (result i32)
        local.get $0
        i32.load
      else
        local.get $0
      end ;; $if_2
      local.tee $3
      local.get $2
      i32.add
      local.set $4
      local.get $1
      if $if_3
        local.get $4
        i32.const 0
        local.get $1
        call $_memset
        drop
      end ;; $if_3
      local.get $1
      local.get $2
      i32.add
      local.set $1
      local.get $0
      i32.load8_s offset=11
      i32.const 0
      i32.lt_s
      if $if_4
        local.get $0
        local.get $1
        i32.store offset=4
      else
        local.get $0
        local.get $1
        i32.store8 offset=11
      end ;; $if_4
      local.get $5
      i32.const 0
      i32.store8
      local.get $1
      local.get $3
      i32.add
      local.get $5
      i32.load8_s
      i32.store8
    end ;; $if
    local.get $5
    global.set $22
    )
  
  (func $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE9__grow_byEmmmmmm (type $6)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (param $4 i32)
    (local $5 i32)
    (local $6 i32)
    i32.const -17
    local.get $1
    i32.sub
    local.get $2
    i32.lt_u
    if $if
      call $_abort
    end ;; $if
    local.get $0
    i32.load8_s offset=11
    i32.const 0
    i32.lt_s
    if $if_0 (result i32)
      local.get $0
      i32.load
    else
      local.get $0
    end ;; $if_0
    local.set $5
    local.get $1
    i32.const 2147483623
    i32.lt_u
    if $if_1 (result i32)
      i32.const 11
      local.get $1
      i32.const 1
      i32.shl
      local.tee $6
      local.get $1
      local.get $2
      i32.add
      local.tee $2
      local.get $2
      local.get $6
      i32.lt_u
      select
      local.tee $2
      i32.const 16
      i32.add
      i32.const -16
      i32.and
      local.get $2
      i32.const 11
      i32.lt_u
      select
    else
      i32.const -17
    end ;; $if_1
    local.tee $6
    call $__Znwm
    local.set $2
    local.get $4
    if $if_2
      local.get $2
      local.get $5
      local.get $4
      call $__ZNSt3__211char_traitsIcE4copyEPcPKcm
    end ;; $if_2
    local.get $3
    local.get $4
    i32.sub
    local.tee $3
    if $if_3
      local.get $2
      local.get $4
      i32.add
      local.get $4
      local.get $5
      i32.add
      local.get $3
      call $__ZNSt3__211char_traitsIcE4copyEPcPKcm
    end ;; $if_3
    local.get $1
    i32.const 10
    i32.ne
    if $if_4
      local.get $5
      call $_free
    end ;; $if_4
    local.get $0
    local.get $2
    i32.store
    local.get $0
    local.get $6
    i32.const -2147483648
    i32.or
    i32.store offset=8
    )
  
  (func $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE7reserveEm (type $7)
    (param $0 i32)
    (local $1 i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    local.get $0
    i32.load8_s offset=11
    local.tee $3
    i32.const 0
    i32.lt_s
    local.tee $6
    if $if (result i32)
      local.get $0
      i32.load offset=8
      i32.const 2147483647
      i32.and
      i32.const -1
      i32.add
      local.set $1
      local.get $0
      i32.load offset=4
    else
      i32.const 10
      local.set $1
      local.get $3
      i32.const 255
      i32.and
    end ;; $if
    local.tee $5
    i32.const 0
    local.get $5
    i32.const 0
    i32.gt_u
    select
    local.tee $4
    i32.const 11
    i32.lt_u
    local.set $2
    i32.const 10
    local.get $4
    i32.const 16
    i32.add
    i32.const -16
    i32.and
    i32.const -1
    i32.add
    local.get $2
    select
    local.tee $4
    local.get $1
    i32.ne
    if $if_0
      block $block
        block $block_0
          block $block_1
            local.get $2
            if $if_1
              local.get $0
              i32.load
              local.set $1
              local.get $6
              if $if_2 (result i32)
                i32.const 0
                local.set $3
                local.get $0
              else
                local.get $0
                local.get $1
                local.get $3
                i32.const 255
                i32.and
                i32.const 1
                i32.add
                call $__ZNSt3__211char_traitsIcE4copyEPcPKcm
                local.get $1
                call $_free
                br $block_0
              end ;; $if_2
              local.set $2
            else
              local.get $4
              i32.const 1
              i32.add
              call $__Znwm
              local.set $2
              local.get $6
              if $if_3 (result i32)
                i32.const 1
                local.set $3
                local.get $0
                i32.load
              else
                local.get $2
                local.get $0
                local.get $3
                i32.const 255
                i32.and
                i32.const 1
                i32.add
                call $__ZNSt3__211char_traitsIcE4copyEPcPKcm
                br $block_1
              end ;; $if_3
              local.set $1
            end ;; $if_1
            local.get $2
            local.get $1
            local.get $0
            i32.load offset=4
            i32.const 1
            i32.add
            call $__ZNSt3__211char_traitsIcE4copyEPcPKcm
            local.get $1
            call $_free
            local.get $3
            i32.eqz
            br_if $block_0
          end ;; $block_1
          local.get $0
          local.get $4
          i32.const 1
          i32.add
          i32.const -2147483648
          i32.or
          i32.store offset=8
          local.get $0
          local.get $5
          i32.store offset=4
          local.get $0
          local.get $2
          i32.store
          br $block
        end ;; $block_0
        local.get $0
        local.get $5
        i32.store8 offset=11
      end ;; $block
    end ;; $if_0
    )
  
  (func $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6insertEmPKcm (type $12)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    i32.const 2093
    local.set $6
    global.get $22
    local.set $5
    global.get $22
    i32.const 16
    i32.add
    global.set $22
    local.get $0
    i32.load8_s offset=11
    local.tee $2
    i32.const 0
    i32.lt_s
    local.tee $4
    if $if (result i32)
      local.get $0
      i32.load offset=4
    else
      local.get $2
      i32.const 255
      i32.and
    end ;; $if
    local.tee $2
    i32.const 0
    i32.lt_u
    if $if_0
      call $_abort
    end ;; $if_0
    local.get $4
    if $if_1 (result i32)
      local.get $0
      i32.load offset=8
      i32.const 2147483647
      i32.and
      i32.const -1
      i32.add
    else
      i32.const 10
    end ;; $if_1
    local.tee $3
    local.get $2
    i32.sub
    local.get $1
    i32.lt_u
    if $if_2
      local.get $0
      local.get $3
      local.get $1
      local.get $2
      i32.add
      local.get $3
      i32.sub
      local.get $2
      local.get $1
      call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE21__grow_by_and_replaceEmmmmmmPKc
    else
      local.get $1
      if $if_3
        local.get $4
        if $if_4 (result i32)
          local.get $0
          i32.load
        else
          local.get $0
        end ;; $if_4
        local.tee $4
        local.set $3
        local.get $2
        if $if_5
          local.get $1
          i32.const 2093
          i32.add
          i32.const 2093
          local.get $3
          i32.const 2093
          i32.le_u
          local.get $2
          local.get $4
          i32.add
          i32.const 2093
          i32.gt_u
          i32.and
          select
          local.set $6
          local.get $1
          local.get $3
          i32.add
          local.get $3
          local.get $2
          call $__ZNSt3__211char_traitsIcE4moveEPcPKcm
        end ;; $if_5
        local.get $3
        local.get $6
        local.get $1
        call $__ZNSt3__211char_traitsIcE4moveEPcPKcm
        local.get $1
        local.get $2
        i32.add
        local.set $1
        local.get $0
        i32.load8_s offset=11
        i32.const 0
        i32.lt_s
        if $if_6
          local.get $0
          local.get $1
          i32.store offset=4
        else
          local.get $0
          local.get $1
          i32.store8 offset=11
        end ;; $if_6
        local.get $5
        i32.const 0
        i32.store8
        local.get $1
        local.get $4
        i32.add
        local.get $5
        i32.load8_s
        i32.store8
      end ;; $if_3
    end ;; $if_2
    local.get $5
    global.set $22
    local.get $0
    )
  
  (func $__ZNSt3__29to_stringEj (type $2)
    (param $0 i32)
    (param $1 i32)
    (local $2 i32)
    global.get $22
    local.set $2
    global.get $22
    i32.const 16
    i32.add
    global.set $22
    local.get $2
    call $__ZNKSt3__212_GLOBAL__N_114initial_stringINS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEjLb0EEclEv
    local.get $0
    local.get $2
    local.get $1
    call $__ZNSt3__212_GLOBAL__N_19as_stringINS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPFiPcmPKczEjEET_T0_SD_PKNSD_10value_typeET1_
    local.get $2
    i32.load8_s offset=11
    i32.const 0
    i32.lt_s
    if $if
      local.get $2
      i32.load offset=8
      drop
      local.get $2
      i32.load
      call $_free
    end ;; $if
    local.get $2
    global.set $22
    )
  
  (func $__ZNKSt3__212_GLOBAL__N_114initial_stringINS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEjLb0EEclEv (type $7)
    (param $0 i32)
    (local $1 i32)
    local.get $0
    i64.const 0
    i64.store align=4
    local.get $0
    i32.const 0
    i32.store offset=8
    loop $loop
      local.get $1
      i32.const 3
      i32.ne
      if $if
        local.get $1
        i32.const 2
        i32.shl
        local.get $0
        i32.add
        i32.const 0
        i32.store
        local.get $1
        i32.const 1
        i32.add
        local.set $1
        br $loop
      end ;; $if
    end ;; $loop
    local.get $0
    local.get $0
    i32.load8_s offset=11
    i32.const 0
    i32.lt_s
    if $if_0 (result i32)
      local.get $0
      i32.load offset=8
      i32.const 2147483647
      i32.and
      i32.const -1
      i32.add
    else
      i32.const 10
    end ;; $if_0
    call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6resizeEmc
    )
  
  (func $__ZNSt3__212_GLOBAL__N_19as_stringINS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPFiPcmPKczEjEET_T0_SD_PKNSD_10value_typeET1_ (type $10)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    global.get $22
    local.set $5
    global.get $22
    i32.const 16
    i32.add
    global.set $22
    local.get $1
    i32.load8_s offset=11
    local.tee $3
    i32.const 0
    i32.lt_s
    if $if (result i32)
      local.get $1
      i32.load offset=4
    else
      local.get $3
      i32.const 255
      i32.and
    end ;; $if
    local.set $4
    loop $loop
      block $block
        local.get $3
        i32.const 24
        i32.shl
        i32.const 24
        i32.shr_s
        i32.const 0
        i32.lt_s
        if $if_0 (result i32)
          local.get $1
          i32.load
        else
          local.get $1
        end ;; $if_0
        local.set $3
        local.get $5
        local.get $2
        i32.store
        local.get $1
        local.get $3
        local.get $4
        i32.const 1
        i32.add
        local.get $5
        call $_snprintf
        local.tee $3
        i32.const -1
        i32.gt_s
        if $if_1 (result i32)
          local.get $3
          local.get $4
          i32.le_u
          br_if $block
          local.get $3
        else
          local.get $4
          i32.const 1
          i32.shl
          i32.const 1
          i32.or
        end ;; $if_1
        local.tee $4
        call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6resizeEmc
        local.get $1
        i32.load8_s offset=11
        local.set $3
        br $loop
      end ;; $block
    end ;; $loop
    local.get $1
    local.get $3
    call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6resizeEmc
    local.get $0
    local.get $1
    i64.load align=4
    i64.store align=4
    local.get $0
    local.get $1
    i32.load offset=8
    i32.store offset=8
    i32.const 0
    local.set $0
    loop $loop_0
      local.get $0
      i32.const 3
      i32.ne
      if $if_2
        local.get $0
        i32.const 2
        i32.shl
        local.get $1
        i32.add
        i32.const 0
        i32.store
        local.get $0
        i32.const 1
        i32.add
        local.set $0
        br $loop_0
      end ;; $if_2
    end ;; $loop_0
    local.get $5
    global.set $22
    )
  
  (func $__ZN10__cxxabiv116__shim_type_infoD2Ev (type $7)
    (param $0 i32)
    nop
    )
  
  (func $__ZN10__cxxabiv117__class_type_infoD0Ev (type $7)
    (param $0 i32)
    local.get $0
    call $_free
    )
  
  (func $__ZNK10__cxxabiv117__class_type_info9can_catchEPKNS_16__shim_type_infoERPv (type $0)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    (local $3 i32)
    global.get $22
    local.set $3
    global.get $22
    i32.const -64
    i32.sub
    global.set $22
    local.get $0
    local.get $1
    i32.const 0
    call $__ZL8is_equalPKSt9type_infoS1_b
    if $if (result i32)
      i32.const 1
    else
      local.get $1
      if $if_0 (result i32)
        local.get $1
        i32.const 1648
        call $___dynamic_cast
        local.tee $1
        if $if_1 (result i32)
          local.get $3
          local.get $1
          i32.store
          local.get $3
          i32.const 0
          i32.store offset=4
          local.get $3
          local.get $0
          i32.store offset=8
          local.get $3
          i32.const -1
          i32.store offset=12
          local.get $3
          i64.const 0
          i64.store offset=16 align=4
          local.get $3
          i64.const 0
          i64.store offset=24 align=4
          local.get $3
          i64.const 0
          i64.store offset=32 align=4
          local.get $3
          i64.const 0
          i64.store offset=40 align=4
          local.get $3
          i32.const 0
          i32.store offset=48
          local.get $3
          i32.const 0
          i32.store16 offset=52
          local.get $3
          i32.const 0
          i32.store8 offset=54
          local.get $3
          i32.const 1
          i32.store offset=48
          local.get $1
          i32.load
          i32.load offset=28
          local.set $0
          local.get $1
          local.get $3
          local.get $2
          i32.load
          i32.const 1
          local.get $0
          i32.const 3
          i32.and
          i32.const 17
          i32.add
          call_indirect $18 (type $4)
          local.get $3
          i32.load offset=24
          i32.const 1
          i32.eq
          if $if_2 (result i32)
            local.get $2
            local.get $3
            i32.load offset=16
            i32.store
            i32.const 1
          else
            i32.const 0
          end ;; $if_2
        else
          i32.const 0
        end ;; $if_1
      else
        i32.const 0
      end ;; $if_0
    end ;; $if
    local.set $0
    local.get $3
    global.set $22
    local.get $0
    )
  
  (func $__ZNK10__cxxabiv117__class_type_info16search_above_dstEPNS_19__dynamic_cast_infoEPKvS4_ib (type $5)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (param $4 i32)
    (param $5 i32)
    local.get $0
    local.get $1
    i32.load offset=8
    local.get $5
    call $__ZL8is_equalPKSt9type_infoS1_b
    if $if
      local.get $1
      local.get $2
      local.get $3
      local.get $4
      call $__ZNK10__cxxabiv117__class_type_info29process_static_type_above_dstEPNS_19__dynamic_cast_infoEPKvS4_i
    end ;; $if
    )
  
  (func $__ZNK10__cxxabiv117__class_type_info16search_below_dstEPNS_19__dynamic_cast_infoEPKvib (type $6)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (param $4 i32)
    local.get $0
    local.get $1
    i32.load offset=8
    local.get $4
    call $__ZL8is_equalPKSt9type_infoS1_b
    if $if
      local.get $2
      local.get $1
      i32.load offset=4
      i32.eq
      if $if_0
        local.get $1
        i32.load offset=28
        i32.const 1
        i32.ne
        if $if_1
          local.get $1
          local.get $3
          i32.store offset=28
        end ;; $if_1
      end ;; $if_0
    else
      local.get $0
      local.get $1
      i32.load
      local.get $4
      call $__ZL8is_equalPKSt9type_infoS1_b
      if $if_2
        block $block
          local.get $1
          i32.load offset=16
          local.get $2
          i32.ne
          if $if_3
            local.get $1
            i32.load offset=20
            local.get $2
            i32.ne
            if $if_4
              local.get $1
              local.get $3
              i32.store offset=32
              local.get $1
              local.get $2
              i32.store offset=20
              local.get $1
              local.get $1
              i32.load offset=40
              i32.const 1
              i32.add
              i32.store offset=40
              local.get $1
              i32.load offset=36
              i32.const 1
              i32.eq
              if $if_5
                local.get $1
                i32.load offset=24
                i32.const 2
                i32.eq
                if $if_6
                  local.get $1
                  i32.const 1
                  i32.store8 offset=54
                end ;; $if_6
              end ;; $if_5
              local.get $1
              i32.const 4
              i32.store offset=44
              br $block
            end ;; $if_4
          end ;; $if_3
          local.get $3
          i32.const 1
          i32.eq
          if $if_7
            local.get $1
            i32.const 1
            i32.store offset=32
          end ;; $if_7
        end ;; $block
      end ;; $if_2
    end ;; $if
    )
  
  (func $__ZNK10__cxxabiv117__class_type_info27has_unambiguous_public_baseEPNS_19__dynamic_cast_infoEPvi (type $4)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    local.get $0
    local.get $1
    i32.load offset=8
    i32.const 0
    call $__ZL8is_equalPKSt9type_infoS1_b
    if $if
      local.get $1
      local.get $2
      local.get $3
      call $__ZNK10__cxxabiv117__class_type_info24process_found_base_classEPNS_19__dynamic_cast_infoEPvi
    end ;; $if
    )
  
  (func $__ZL8is_equalPKSt9type_infoS1_b (type $0)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    local.get $2
    if $if (result i32)
      local.get $0
      i32.load offset=4
      local.get $1
      i32.load offset=4
      call $_strcmp
      i32.eqz
    else
      local.get $0
      local.get $1
      i32.eq
    end ;; $if
    )
  
  (func $__ZNK10__cxxabiv117__class_type_info24process_found_base_classEPNS_19__dynamic_cast_infoEPvi (type $10)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (local $3 i32)
    local.get $0
    i32.load offset=16
    local.tee $3
    if $if
      block $block
        local.get $1
        local.get $3
        i32.ne
        if $if_0
          local.get $0
          local.get $0
          i32.load offset=36
          i32.const 1
          i32.add
          i32.store offset=36
          local.get $0
          i32.const 2
          i32.store offset=24
          local.get $0
          i32.const 1
          i32.store8 offset=54
          br $block
        end ;; $if_0
        local.get $0
        i32.load offset=24
        i32.const 2
        i32.eq
        if $if_1
          local.get $0
          local.get $2
          i32.store offset=24
        end ;; $if_1
      end ;; $block
    else
      local.get $0
      local.get $1
      i32.store offset=16
      local.get $0
      local.get $2
      i32.store offset=24
      local.get $0
      i32.const 1
      i32.store offset=36
    end ;; $if
    )
  
  (func $__ZNK10__cxxabiv117__class_type_info29process_static_type_above_dstEPNS_19__dynamic_cast_infoEPKvS4_i (type $4)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    local.get $0
    i32.const 1
    i32.store8 offset=53
    local.get $2
    local.get $0
    i32.load offset=4
    i32.eq
    if $if
      block $block
        local.get $0
        i32.const 1
        i32.store8 offset=52
        local.get $0
        i32.load offset=16
        local.tee $2
        i32.eqz
        if $if_0
          local.get $0
          local.get $1
          i32.store offset=16
          local.get $0
          local.get $3
          i32.store offset=24
          local.get $0
          i32.const 1
          i32.store offset=36
          local.get $0
          i32.load offset=48
          i32.const 1
          i32.eq
          local.get $3
          i32.const 1
          i32.eq
          i32.and
          i32.eqz
          br_if $block
          local.get $0
          i32.const 1
          i32.store8 offset=54
          br $block
        end ;; $if_0
        local.get $1
        local.get $2
        i32.ne
        if $if_1
          local.get $0
          local.get $0
          i32.load offset=36
          i32.const 1
          i32.add
          i32.store offset=36
          local.get $0
          i32.const 1
          i32.store8 offset=54
          br $block
        end ;; $if_1
        local.get $0
        i32.load offset=24
        local.tee $1
        i32.const 2
        i32.eq
        if $if_2
          local.get $0
          local.get $3
          i32.store offset=24
        else
          local.get $1
          local.set $3
        end ;; $if_2
        local.get $0
        i32.load offset=48
        i32.const 1
        i32.eq
        local.get $3
        i32.const 1
        i32.eq
        i32.and
        if $if_3
          local.get $0
          i32.const 1
          i32.store8 offset=54
        end ;; $if_3
      end ;; $block
    end ;; $if
    )
  
  (func $___dynamic_cast (type $12)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    global.get $22
    local.set $2
    global.get $22
    i32.const -64
    i32.sub
    global.set $22
    local.get $0
    local.get $0
    i32.load
    local.tee $3
    i32.const -8
    i32.add
    i32.load
    i32.add
    local.set $4
    local.get $3
    i32.const -4
    i32.add
    i32.load
    local.set $3
    local.get $2
    local.get $1
    i32.store
    local.get $2
    local.get $0
    i32.store offset=4
    local.get $2
    i32.const 1664
    i32.store offset=8
    local.get $2
    i32.const 0
    i32.store offset=12
    local.get $2
    i64.const 0
    i64.store offset=16 align=4
    local.get $2
    i64.const 0
    i64.store offset=24 align=4
    local.get $2
    i64.const 0
    i64.store offset=32 align=4
    local.get $2
    i64.const 0
    i64.store offset=40 align=4
    local.get $2
    i32.const 0
    i32.store offset=48
    local.get $2
    i32.const 0
    i32.store16 offset=52
    local.get $2
    i32.const 0
    i32.store8 offset=54
    local.get $3
    local.get $1
    i32.const 0
    call $__ZL8is_equalPKSt9type_infoS1_b
    if $if (result i32)
      local.get $2
      i32.const 1
      i32.store offset=48
      local.get $3
      local.get $2
      local.get $4
      local.get $4
      i32.const 1
      i32.const 0
      local.get $3
      i32.load
      i32.load offset=20
      i32.const 3
      i32.and
      i32.const 25
      i32.add
      call_indirect $18 (type $5)
      local.get $4
      i32.const 0
      local.get $2
      i32.load offset=24
      i32.const 1
      i32.eq
      select
    else
      block $block (result i32)
        local.get $3
        local.get $2
        local.get $4
        i32.const 1
        i32.const 0
        local.get $3
        i32.load
        i32.load offset=24
        i32.const 3
        i32.and
        i32.const 21
        i32.add
        call_indirect $18 (type $6)
        block $block_0
          block $block_1
            block $block_2
              local.get $2
              i32.load offset=36
              br_table
                $block_2 $block_0
                $block_1 ;; default
            end ;; $block_2
            local.get $2
            i32.load offset=20
            i32.const 0
            local.get $2
            i32.load offset=40
            i32.const 1
            i32.eq
            local.get $2
            i32.load offset=28
            i32.const 1
            i32.eq
            i32.and
            local.get $2
            i32.load offset=32
            i32.const 1
            i32.eq
            i32.and
            select
            br $block
          end ;; $block_1
          i32.const 0
          br $block
        end ;; $block_0
        local.get $2
        i32.load offset=24
        i32.const 1
        i32.ne
        if $if_0
          i32.const 0
          local.get $2
          i32.load offset=40
          i32.eqz
          local.get $2
          i32.load offset=28
          i32.const 1
          i32.eq
          i32.and
          local.get $2
          i32.load offset=32
          i32.const 1
          i32.eq
          i32.and
          i32.eqz
          br_if $block
          drop
        end ;; $if_0
        local.get $2
        i32.load offset=16
      end ;; $block
    end ;; $if
    local.set $0
    local.get $2
    global.set $22
    local.get $0
    )
  
  (func $__ZNK10__cxxabiv120__si_class_type_info16search_above_dstEPNS_19__dynamic_cast_infoEPKvS4_ib (type $5)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (param $4 i32)
    (param $5 i32)
    (local $6 i32)
    local.get $0
    local.get $1
    i32.load offset=8
    local.get $5
    call $__ZL8is_equalPKSt9type_infoS1_b
    if $if
      local.get $1
      local.get $2
      local.get $3
      local.get $4
      call $__ZNK10__cxxabiv117__class_type_info29process_static_type_above_dstEPNS_19__dynamic_cast_infoEPKvS4_i
    else
      local.get $0
      i32.load offset=8
      local.tee $0
      i32.load
      i32.load offset=20
      local.set $6
      local.get $0
      local.get $1
      local.get $2
      local.get $3
      local.get $4
      local.get $5
      local.get $6
      i32.const 3
      i32.and
      i32.const 25
      i32.add
      call_indirect $18 (type $5)
    end ;; $if
    )
  
  (func $__ZNK10__cxxabiv120__si_class_type_info16search_below_dstEPNS_19__dynamic_cast_infoEPKvib (type $6)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (param $4 i32)
    (local $5 i32)
    local.get $0
    local.get $1
    i32.load offset=8
    local.get $4
    call $__ZL8is_equalPKSt9type_infoS1_b
    if $if
      local.get $2
      local.get $1
      i32.load offset=4
      i32.eq
      if $if_0
        local.get $1
        i32.load offset=28
        i32.const 1
        i32.ne
        if $if_1
          local.get $1
          local.get $3
          i32.store offset=28
        end ;; $if_1
      end ;; $if_0
    else
      block $block
        local.get $0
        local.get $1
        i32.load
        local.get $4
        call $__ZL8is_equalPKSt9type_infoS1_b
        i32.eqz
        if $if_2
          local.get $0
          i32.load offset=8
          local.tee $0
          i32.load
          i32.load offset=24
          local.set $5
          local.get $0
          local.get $1
          local.get $2
          local.get $3
          local.get $4
          local.get $5
          i32.const 3
          i32.and
          i32.const 21
          i32.add
          call_indirect $18 (type $6)
          br $block
        end ;; $if_2
        local.get $1
        i32.load offset=16
        local.get $2
        i32.ne
        if $if_3
          local.get $1
          i32.load offset=20
          local.get $2
          i32.ne
          if $if_4
            local.get $1
            local.get $3
            i32.store offset=32
            local.get $1
            i32.load offset=44
            i32.const 4
            i32.ne
            if $if_5
              local.get $1
              i32.const 0
              i32.store8 offset=52
              local.get $1
              i32.const 0
              i32.store8 offset=53
              local.get $0
              i32.load offset=8
              local.tee $0
              i32.load
              i32.load offset=20
              local.set $3
              local.get $0
              local.get $1
              local.get $2
              local.get $2
              i32.const 1
              local.get $4
              local.get $3
              i32.const 3
              i32.and
              i32.const 25
              i32.add
              call_indirect $18 (type $5)
              local.get $1
              i32.load8_s offset=53
              if $if_6
                local.get $1
                i32.load8_s offset=52
                i32.eqz
                local.set $0
                local.get $1
                i32.const 3
                i32.store offset=44
                local.get $0
                i32.eqz
                br_if $block
              else
                local.get $1
                i32.const 4
                i32.store offset=44
              end ;; $if_6
            end ;; $if_5
            local.get $1
            local.get $2
            i32.store offset=20
            local.get $1
            local.get $1
            i32.load offset=40
            i32.const 1
            i32.add
            i32.store offset=40
            local.get $1
            i32.load offset=36
            i32.const 1
            i32.ne
            br_if $block
            local.get $1
            i32.load offset=24
            i32.const 2
            i32.ne
            br_if $block
            local.get $1
            i32.const 1
            i32.store8 offset=54
            br $block
          end ;; $if_4
        end ;; $if_3
        local.get $3
        i32.const 1
        i32.eq
        if $if_7
          local.get $1
          i32.const 1
          i32.store offset=32
        end ;; $if_7
      end ;; $block
    end ;; $if
    )
  
  (func $__ZNK10__cxxabiv120__si_class_type_info27has_unambiguous_public_baseEPNS_19__dynamic_cast_infoEPvi (type $4)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (local $4 i32)
    local.get $0
    local.get $1
    i32.load offset=8
    i32.const 0
    call $__ZL8is_equalPKSt9type_infoS1_b
    if $if
      local.get $1
      local.get $2
      local.get $3
      call $__ZNK10__cxxabiv117__class_type_info24process_found_base_classEPNS_19__dynamic_cast_infoEPvi
    else
      local.get $0
      i32.load offset=8
      local.tee $0
      i32.load
      i32.load offset=28
      local.set $4
      local.get $0
      local.get $1
      local.get $2
      local.get $3
      local.get $4
      i32.const 3
      i32.and
      i32.const 17
      i32.add
      call_indirect $18 (type $4)
    end ;; $if
    )
  
  (func $___cxa_can_catch (type $0)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    (local $3 i32)
    (local $4 i32)
    global.get $22
    local.set $3
    global.get $22
    i32.const 16
    i32.add
    global.set $22
    local.get $3
    local.get $2
    i32.load
    i32.store
    local.get $0
    i32.load
    i32.load offset=16
    local.set $4
    local.get $0
    local.get $1
    local.get $3
    local.get $4
    i32.const 3
    i32.and
    i32.const 2
    i32.add
    call_indirect $18 (type $0)
    local.tee $0
    if $if
      local.get $2
      local.get $3
      i32.load
      i32.store
    end ;; $if
    local.get $3
    global.set $22
    local.get $0
    i32.const 1
    i32.and
    )
  
  (func $___cxa_is_pointer_type (type $9)
    (param $0 i32)
    (result i32)
    local.get $0
    if $if (result i32)
      local.get $0
      i32.const 1720
      call $___dynamic_cast
      i32.const 0
      i32.ne
    else
      i32.const 0
    end ;; $if
    )
  
  (func $_memcpy (type $0)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    local.get $2
    i32.const 8192
    i32.ge_s
    if $if
      local.get $0
      local.get $1
      local.get $2
      call $_emscripten_memcpy_big
      drop
      local.get $0
      return
    end ;; $if
    local.get $0
    local.set $4
    local.get $0
    local.get $2
    i32.add
    local.set $3
    local.get $0
    i32.const 3
    i32.and
    local.get $1
    i32.const 3
    i32.and
    i32.eq
    if $if_0
      loop $loop
        local.get $0
        i32.const 3
        i32.and
        if $if_1
          local.get $2
          i32.eqz
          if $if_2
            local.get $4
            return
          end ;; $if_2
          local.get $0
          local.get $1
          i32.load8_s
          i32.store8
          local.get $0
          i32.const 1
          i32.add
          local.set $0
          local.get $1
          i32.const 1
          i32.add
          local.set $1
          local.get $2
          i32.const 1
          i32.sub
          local.set $2
          br $loop
        end ;; $if_1
      end ;; $loop
      local.get $3
      i32.const -4
      i32.and
      local.tee $2
      i32.const -64
      i32.add
      local.set $5
      loop $loop_0
        local.get $0
        local.get $5
        i32.le_s
        if $if_3
          local.get $0
          local.get $1
          i32.load
          i32.store
          local.get $0
          local.get $1
          i32.load offset=4
          i32.store offset=4
          local.get $0
          local.get $1
          i32.load offset=8
          i32.store offset=8
          local.get $0
          local.get $1
          i32.load offset=12
          i32.store offset=12
          local.get $0
          local.get $1
          i32.load offset=16
          i32.store offset=16
          local.get $0
          local.get $1
          i32.load offset=20
          i32.store offset=20
          local.get $0
          local.get $1
          i32.load offset=24
          i32.store offset=24
          local.get $0
          local.get $1
          i32.load offset=28
          i32.store offset=28
          local.get $0
          local.get $1
          i32.load offset=32
          i32.store offset=32
          local.get $0
          local.get $1
          i32.load offset=36
          i32.store offset=36
          local.get $0
          local.get $1
          i32.load offset=40
          i32.store offset=40
          local.get $0
          local.get $1
          i32.load offset=44
          i32.store offset=44
          local.get $0
          local.get $1
          i32.load offset=48
          i32.store offset=48
          local.get $0
          local.get $1
          i32.load offset=52
          i32.store offset=52
          local.get $0
          local.get $1
          i32.load offset=56
          i32.store offset=56
          local.get $0
          local.get $1
          i32.load offset=60
          i32.store offset=60
          local.get $0
          i32.const -64
          i32.sub
          local.set $0
          local.get $1
          i32.const -64
          i32.sub
          local.set $1
          br $loop_0
        end ;; $if_3
      end ;; $loop_0
      loop $loop_1
        local.get $0
        local.get $2
        i32.lt_s
        if $if_4
          local.get $0
          local.get $1
          i32.load
          i32.store
          local.get $0
          i32.const 4
          i32.add
          local.set $0
          local.get $1
          i32.const 4
          i32.add
          local.set $1
          br $loop_1
        end ;; $if_4
      end ;; $loop_1
    else
      local.get $3
      i32.const 4
      i32.sub
      local.set $2
      loop $loop_2
        local.get $0
        local.get $2
        i32.lt_s
        if $if_5
          local.get $0
          local.get $1
          i32.load8_s
          i32.store8
          local.get $0
          local.get $1
          i32.load8_s offset=1
          i32.store8 offset=1
          local.get $0
          local.get $1
          i32.load8_s offset=2
          i32.store8 offset=2
          local.get $0
          local.get $1
          i32.load8_s offset=3
          i32.store8 offset=3
          local.get $0
          i32.const 4
          i32.add
          local.set $0
          local.get $1
          i32.const 4
          i32.add
          local.set $1
          br $loop_2
        end ;; $if_5
      end ;; $loop_2
    end ;; $if_0
    loop $loop_3
      local.get $0
      local.get $3
      i32.lt_s
      if $if_6
        local.get $0
        local.get $1
        i32.load8_s
        i32.store8
        local.get $0
        i32.const 1
        i32.add
        local.set $0
        local.get $1
        i32.const 1
        i32.add
        local.set $1
        br $loop_3
      end ;; $if_6
    end ;; $loop_3
    local.get $4
    )
  
  (func $_memmove (type $0)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    (local $3 i32)
    local.get $1
    local.get $0
    i32.lt_s
    local.get $0
    local.get $1
    local.get $2
    i32.add
    i32.lt_s
    i32.and
    if $if
      local.get $1
      local.get $2
      i32.add
      local.set $1
      local.get $0
      local.tee $3
      local.get $2
      i32.add
      local.set $0
      loop $loop
        local.get $2
        i32.const 0
        i32.gt_s
        if $if_0
          local.get $2
          i32.const 1
          i32.sub
          local.set $2
          local.get $0
          i32.const 1
          i32.sub
          local.tee $0
          local.get $1
          i32.const 1
          i32.sub
          local.tee $1
          i32.load8_s
          i32.store8
          br $loop
        end ;; $if_0
      end ;; $loop
      local.get $3
      local.set $0
    else
      local.get $0
      local.get $1
      local.get $2
      call $_memcpy
      drop
    end ;; $if
    local.get $0
    )
  
  (func $_memset (type $0)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    local.get $0
    local.get $2
    i32.add
    local.set $4
    local.get $1
    i32.const 255
    i32.and
    local.set $3
    local.get $2
    i32.const 67
    i32.ge_s
    if $if
      loop $loop
        local.get $0
        i32.const 3
        i32.and
        if $if_0
          local.get $0
          local.get $3
          i32.store8
          local.get $0
          i32.const 1
          i32.add
          local.set $0
          br $loop
        end ;; $if_0
      end ;; $loop
      local.get $3
      i32.const 8
      i32.shl
      local.get $3
      i32.or
      local.get $3
      i32.const 16
      i32.shl
      i32.or
      local.get $3
      i32.const 24
      i32.shl
      i32.or
      local.set $1
      local.get $4
      i32.const -4
      i32.and
      local.tee $5
      i32.const -64
      i32.add
      local.set $6
      loop $loop_0
        local.get $0
        local.get $6
        i32.le_s
        if $if_1
          local.get $0
          local.get $1
          i32.store
          local.get $0
          local.get $1
          i32.store offset=4
          local.get $0
          local.get $1
          i32.store offset=8
          local.get $0
          local.get $1
          i32.store offset=12
          local.get $0
          local.get $1
          i32.store offset=16
          local.get $0
          local.get $1
          i32.store offset=20
          local.get $0
          local.get $1
          i32.store offset=24
          local.get $0
          local.get $1
          i32.store offset=28
          local.get $0
          local.get $1
          i32.store offset=32
          local.get $0
          local.get $1
          i32.store offset=36
          local.get $0
          local.get $1
          i32.store offset=40
          local.get $0
          local.get $1
          i32.store offset=44
          local.get $0
          local.get $1
          i32.store offset=48
          local.get $0
          local.get $1
          i32.store offset=52
          local.get $0
          local.get $1
          i32.store offset=56
          local.get $0
          local.get $1
          i32.store offset=60
          local.get $0
          i32.const -64
          i32.sub
          local.set $0
          br $loop_0
        end ;; $if_1
      end ;; $loop_0
      loop $loop_1
        local.get $0
        local.get $5
        i32.lt_s
        if $if_2
          local.get $0
          local.get $1
          i32.store
          local.get $0
          i32.const 4
          i32.add
          local.set $0
          br $loop_1
        end ;; $if_2
      end ;; $loop_1
    end ;; $if
    loop $loop_2
      local.get $0
      local.get $4
      i32.lt_s
      if $if_3
        local.get $0
        local.get $3
        i32.store8
        local.get $0
        i32.const 1
        i32.add
        local.set $0
        br $loop_2
      end ;; $if_3
    end ;; $loop_2
    local.get $4
    local.get $2
    i32.sub
    )
  
  (func $_sbrk (type $9)
    (param $0 i32)
    (result i32)
    (local $1 i32)
    (local $2 i32)
    (local $3 i32)
    call $_emscripten_get_heap_size
    local.set $3
    local.get $0
    global.get $21
    i32.load
    local.tee $2
    i32.add
    local.tee $1
    local.get $2
    i32.lt_s
    local.get $0
    i32.const 0
    i32.gt_s
    i32.and
    local.get $1
    i32.const 0
    i32.lt_s
    i32.or
    if $if
      local.get $1
      call $abortOnCannotGrowMemory
      drop
      i32.const 12
      call $___setErrNo
      i32.const -1
      return
    end ;; $if
    local.get $1
    local.get $3
    i32.gt_s
    if $if_0
      local.get $1
      call $_emscripten_resize_heap
      i32.eqz
      if $if_1
        i32.const 12
        call $___setErrNo
        i32.const -1
        return
      end ;; $if_1
    end ;; $if_0
    global.get $21
    local.get $1
    i32.store
    local.get $2
    )
  
  (func $dynCall_iidiiii (type $16)
    (param $0 i32)
    (param $1 i32)
    (param $2 f64)
    (param $3 i32)
    (param $4 i32)
    (param $5 i32)
    (param $6 i32)
    (result i32)
    local.get $1
    local.get $2
    local.get $3
    local.get $4
    local.get $5
    local.get $6
    local.get $0
    i32.const 1
    i32.and
    call_indirect $18 (type $1)
    )
  
  (func $dynCall_iiii (type $13)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (result i32)
    local.get $1
    local.get $2
    local.get $3
    local.get $0
    i32.const 3
    i32.and
    i32.const 2
    i32.add
    call_indirect $18 (type $0)
    )
  
  (func $dynCall_v (type $7)
    (param $0 i32)
    i32.const 6
    call_indirect $18 (type $3)
    )
  
  (func $dynCall_vi (type $2)
    (param $0 i32)
    (param $1 i32)
    local.get $1
    local.get $0
    i32.const 7
    i32.and
    i32.const 7
    i32.add
    call_indirect $18 (type $7)
    )
  
  (func $dynCall_vii (type $10)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    local.get $1
    local.get $2
    local.get $0
    i32.const 1
    i32.and
    i32.const 15
    i32.add
    call_indirect $18 (type $2)
    )
  
  (func $dynCall_viiii (type $6)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (param $4 i32)
    local.get $1
    local.get $2
    local.get $3
    local.get $4
    local.get $0
    i32.const 3
    i32.and
    i32.const 17
    i32.add
    call_indirect $18 (type $4)
    )
  
  (func $dynCall_viiiii (type $5)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (param $4 i32)
    (param $5 i32)
    local.get $1
    local.get $2
    local.get $3
    local.get $4
    local.get $5
    local.get $0
    i32.const 3
    i32.and
    i32.const 21
    i32.add
    call_indirect $18 (type $6)
    )
  
  (func $dynCall_viiiiii (type $17)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (param $4 i32)
    (param $5 i32)
    (param $6 i32)
    local.get $1
    local.get $2
    local.get $3
    local.get $4
    local.get $5
    local.get $6
    local.get $0
    i32.const 3
    i32.and
    i32.const 25
    i32.add
    call_indirect $18 (type $5)
    )
  
  (func $b0 (type $1)
    (param $0 i32)
    (param $1 f64)
    (param $2 i32)
    (param $3 i32)
    (param $4 i32)
    (param $5 i32)
    (result i32)
    i32.const 0
    call $abort
    i32.const 0
    )
  
  (func $b1 (type $0)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    i32.const 1
    call $abort
    i32.const 0
    )
  
  (func $b2 (type $3)
    i32.const 2
    call $abort
    )
  
  (func $b3 (type $7)
    (param $0 i32)
    i32.const 3
    call $abort
    )
  
  (func $b4 (type $2)
    (param $0 i32)
    (param $1 i32)
    i32.const 4
    call $abort
    )
  
  (func $b5 (type $4)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    i32.const 5
    call $abort
    )
  
  (func $b6 (type $6)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (param $4 i32)
    i32.const 6
    call $abort
    )
  
  (func $b7 (type $5)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (param $4 i32)
    (param $5 i32)
    i32.const 7
    call $abort
    )
  ;; User section "emscripten_metadata":
    ;; "\00\01\00\03\80\02\1d\80\08\90\a1\c0\02\f0 \80!"
  )