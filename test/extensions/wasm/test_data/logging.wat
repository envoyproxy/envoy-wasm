(module
  (type $0 (func))
  (type $1 (func (param i32)))
  (type $2 (func (result i32)))
  (type $3 (func (param i32 i32 i32) (result i32)))
  (type $4 (func (param i32 i32 i32)))
  (type $5 (func (param i32) (result i32)))
  (type $6 (func (param i32 i32)))
  (type $7 (func (param i32 i32) (result i32)))
  (type $8 (func (param i32 i32 i32 i32 i32 i32 i32)))
  (type $9 (func (param i32 i32 i32 i32) (result i32)))
  (import "env" "abort" (func $abort (param i32)))
  (import "env" "enlargeMemory" (func $enlargeMemory (result i32)))
  (import "env" "getTotalMemory" (func $getTotalMemory (result i32)))
  (import "env" "abortOnCannotGrowMemory" (func $abortOnCannotGrowMemory (result i32)))
  (import "env" "___setErrNo" (func $___setErrNo (param i32)))
  (import "env" "_abort" (func $_abort))
  (import "env" "_emscripten_memcpy_big" (func $_emscripten_memcpy_big (param i32 i32 i32) (result i32)))
  (import "env" "_envoy_log" (func $_envoy_log (param i32 i32 i32)))
  (import "env" "table" (table $10 1 1 anyfunc))
  (import "env" "memory" (memory $11 256 256))
  (import "env" "__table_base" (global $12 i32))
  (import "env" "DYNAMICTOP_PTR" (global $13 i32))
  (export "___errno_location" (func $___errno_location))
  (export "_configure" (func $_configure))
  (export "_free" (func $_free))
  (export "_main" (func $_main))
  (export "_malloc" (func $_malloc))
  (export "_memcpy" (func $_memcpy))
  (export "_memmove" (func $_memmove))
  (export "_memset" (func $_memset))
  (export "_sbrk" (func $_sbrk))
  (export "_tick" (func $_tick))
  (export "dynCall_v" (func $dynCall_v))
  (export "establishStackSpace" (func $establishStackSpace))
  (export "setThrew" (func $setThrew))
  (export "stackAlloc" (func $stackAlloc))
  (export "stackRestore" (func $stackRestore))
  (export "stackSave" (func $stackSave))
  (global $14  (mut i32) (get_global $13))
  (global $15  (mut i32) (i32.const 0))
  (global $16  (mut i32) (i32.const 0))
  (global $17  (mut i32) (i32.const 2864))
  (global $18  (mut i32) (i32.const 5245744))
  (elem $10 (get_global $12)
    $b0)
  (data $11 (i32.const 1024)
    "warn \00test tick\00 logging\00test debug\00test info")
  
  (func $stackAlloc (type $5)
    (param $0 i32)
    (result i32)
    (local $1 i32)
    get_global $17
    set_local $1
    get_local $0
    get_global $17
    i32.add
    set_global $17
    get_global $17
    i32.const 15
    i32.add
    i32.const -16
    i32.and
    set_global $17
    get_local $1
    )
  
  (func $stackSave (type $2)
    (result i32)
    get_global $17
    )
  
  (func $stackRestore (type $1)
    (param $0 i32)
    get_local $0
    set_global $17
    )
  
  (func $establishStackSpace (type $6)
    (param $0 i32)
    (param $1 i32)
    get_local $0
    set_global $17
    get_local $1
    set_global $18
    )
  
  (func $setThrew (type $6)
    (param $0 i32)
    (param $1 i32)
    get_global $15
    i32.eqz
    if $if
      get_local $0
      set_global $15
      get_local $1
      set_global $16
    end ;; $if
    )
  
  (func $_configure (type $7)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    get_global $17
    set_local $5
    get_global $17
    i32.const 32
    i32.add
    set_global $17
    get_local $5
    tee_local $2
    i64.const 0
    i64.store align=4
    get_local $2
    i32.const 0
    i32.store offset=8
    get_local $1
    i32.const -17
    i32.gt_u
    if $if
      call $__ZNKSt3__221__basic_string_commonILb1EE20__throw_length_errorEv
    end ;; $if
    get_local $5
    i32.const 12
    i32.add
    set_local $3
    block $block
      block $block_0
        get_local $1
        i32.const 11
        i32.lt_u
        if $if_0 (result i32)
          get_local $2
          get_local $1
          i32.store8 offset=11
          get_local $1
          if $if_1 (result i32)
            get_local $2
            set_local $4
            br $block_0
          else
            get_local $2
          end ;; $if_1
        else
          get_local $2
          get_local $1
          i32.const 16
          i32.add
          i32.const -16
          i32.and
          tee_local $6
          call $__Znwm
          tee_local $4
          i32.store
          get_local $2
          get_local $6
          i32.const -2147483648
          i32.or
          i32.store offset=8
          get_local $2
          get_local $1
          i32.store offset=4
          br $block_0
        end ;; $if_0
        set_local $4
        br $block
      end ;; $block_0
      get_local $4
      get_local $0
      get_local $1
      call $_memcpy
      drop
    end ;; $block
    get_local $1
    get_local $4
    i32.add
    i32.const 0
    i32.store8
    get_local $3
    get_local $2
    call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6insertEmPKc
    tee_local $0
    i64.load align=4
    i64.store align=4
    get_local $3
    get_local $0
    i32.load offset=8
    i32.store offset=8
    get_local $0
    i64.const 0
    i64.store align=4
    get_local $0
    i32.const 0
    i32.store offset=8
    i32.const 3
    get_local $3
    i32.load
    get_local $3
    get_local $3
    i32.const 11
    i32.add
    tee_local $4
    i32.load8_s
    tee_local $1
    i32.const 0
    i32.lt_s
    tee_local $0
    select
    get_local $3
    i32.load offset=4
    get_local $1
    i32.const 255
    i32.and
    get_local $0
    select
    call $_envoy_log
    get_local $4
    i32.load8_s
    i32.const 0
    i32.lt_s
    if $if_2
      get_local $3
      i32.load
      call $__ZdlPv
    end ;; $if_2
    get_local $2
    i32.load8_s offset=11
    i32.const 0
    i32.ge_s
    if $if_3
      get_local $5
      set_global $17
      i32.const 0
      return
    end ;; $if_3
    get_local $2
    i32.load
    call $__ZdlPv
    get_local $5
    set_global $17
    i32.const 0
    )
  
  (func $_tick (type $2)
    (result i32)
    (local $0 i32)
    (local $1 i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    get_global $17
    set_local $0
    get_global $17
    i32.const 32
    i32.add
    set_global $17
    get_local $0
    i64.const 0
    i64.store align=4
    get_local $0
    i32.const 0
    i32.store offset=8
    get_local $0
    i32.const 11
    i32.add
    tee_local $3
    i32.const 9
    i32.store8
    get_local $0
    i32.const 1030
    i64.load align=1
    i64.store align=1
    get_local $0
    i32.const 1038
    i32.load8_s
    i32.store8 offset=8
    get_local $0
    i32.const 0
    i32.store8 offset=9
    get_local $0
    i32.const 12
    i32.add
    tee_local $1
    get_local $0
    call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6appendEPKc
    tee_local $2
    i64.load align=4
    i64.store align=4
    get_local $1
    get_local $2
    i32.load offset=8
    i32.store offset=8
    get_local $2
    i64.const 0
    i64.store align=4
    get_local $2
    i32.const 0
    i32.store offset=8
    i32.const 4
    get_local $1
    i32.load
    get_local $1
    get_local $1
    i32.const 11
    i32.add
    tee_local $4
    i32.load8_s
    tee_local $5
    i32.const 0
    i32.lt_s
    tee_local $2
    select
    get_local $1
    i32.load offset=4
    get_local $5
    i32.const 255
    i32.and
    get_local $2
    select
    call $_envoy_log
    get_local $4
    i32.load8_s
    i32.const 0
    i32.lt_s
    if $if
      get_local $1
      i32.load
      call $__ZdlPv
    end ;; $if
    get_local $3
    i32.load8_s
    i32.const 0
    i32.ge_s
    if $if_0
      get_local $0
      set_global $17
      i32.const 0
      return
    end ;; $if_0
    get_local $0
    i32.load
    call $__ZdlPv
    get_local $0
    set_global $17
    i32.const 0
    )
  
  (func $_main (type $2)
    (result i32)
    (local $0 i32)
    (local $1 i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    get_global $17
    set_local $0
    get_global $17
    i32.const 32
    i32.add
    set_global $17
    get_local $0
    i32.const 11
    i32.add
    tee_local $3
    i32.const 10
    i32.store8
    get_local $0
    i32.const 1049
    i64.load align=1
    i64.store align=1
    get_local $0
    i32.const 1057
    i32.load16_s align=1
    i32.store16 offset=8 align=1
    get_local $0
    i32.const 0
    i32.store8 offset=10
    get_local $0
    i32.const 12
    i32.add
    tee_local $1
    get_local $0
    call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6appendEPKc
    tee_local $2
    i64.load align=4
    i64.store align=4
    get_local $1
    get_local $2
    i32.load offset=8
    i32.store offset=8
    get_local $2
    i64.const 0
    i64.store align=4
    get_local $2
    i32.const 0
    i32.store offset=8
    i32.const 1
    get_local $1
    i32.load
    get_local $1
    get_local $1
    i32.const 11
    i32.add
    tee_local $4
    i32.load8_s
    tee_local $5
    i32.const 0
    i32.lt_s
    tee_local $2
    select
    get_local $1
    i32.load offset=4
    get_local $5
    i32.const 255
    i32.and
    get_local $2
    select
    call $_envoy_log
    get_local $4
    i32.load8_s
    i32.const 0
    i32.lt_s
    if $if
      get_local $1
      i32.load
      call $__ZdlPv
    end ;; $if
    get_local $3
    i32.load8_s
    i32.const 0
    i32.lt_s
    if $if_0
      get_local $0
      i32.load
      call $__ZdlPv
    end ;; $if_0
    get_local $0
    i64.const 0
    i64.store align=4
    get_local $0
    i32.const 0
    i32.store offset=8
    get_local $0
    i32.const 11
    i32.add
    tee_local $3
    i32.const 9
    i32.store8
    get_local $0
    i32.const 1060
    i64.load align=1
    i64.store align=1
    get_local $0
    i32.const 1068
    i32.load8_s
    i32.store8 offset=8
    get_local $0
    i32.const 0
    i32.store8 offset=9
    get_local $1
    get_local $0
    call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6appendEPKc
    tee_local $2
    i64.load align=4
    i64.store align=4
    get_local $1
    get_local $2
    i32.load offset=8
    i32.store offset=8
    get_local $2
    i64.const 0
    i64.store align=4
    get_local $2
    i32.const 0
    i32.store offset=8
    i32.const 2
    get_local $1
    i32.load
    get_local $1
    get_local $1
    i32.const 11
    i32.add
    tee_local $4
    i32.load8_s
    tee_local $5
    i32.const 0
    i32.lt_s
    tee_local $2
    select
    get_local $1
    i32.load offset=4
    get_local $5
    i32.const 255
    i32.and
    get_local $2
    select
    call $_envoy_log
    get_local $4
    i32.load8_s
    i32.const 0
    i32.lt_s
    if $if_1
      get_local $1
      i32.load
      call $__ZdlPv
    end ;; $if_1
    get_local $3
    i32.load8_s
    i32.const 0
    i32.ge_s
    if $if_2
      get_local $0
      set_global $17
      i32.const 0
      return
    end ;; $if_2
    get_local $0
    i32.load
    call $__ZdlPv
    get_local $0
    set_global $17
    i32.const 0
    )
  
  (func $___errno_location (type $2)
    (result i32)
    i32.const 1072
    )
  
  (func $_strlen (type $5)
    (param $0 i32)
    (result i32)
    (local $1 i32)
    (local $2 i32)
    (local $3 i32)
    block $block
      block $block_0
        get_local $0
        tee_local $2
        i32.const 3
        i32.and
        i32.eqz
        br_if $block_0
        get_local $2
        tee_local $1
        set_local $0
        block $block_1
          loop $loop
            get_local $1
            i32.load8_s
            i32.eqz
            br_if $block_1
            get_local $1
            i32.const 1
            i32.add
            tee_local $1
            tee_local $0
            i32.const 3
            i32.and
            br_if $loop
          end ;; $loop
          get_local $1
          set_local $0
          br $block_0
        end ;; $block_1
        br $block
      end ;; $block_0
      loop $loop_0
        get_local $0
        i32.const 4
        i32.add
        set_local $1
        get_local $0
        i32.load
        tee_local $3
        i32.const -16843009
        i32.add
        get_local $3
        i32.const -2139062144
        i32.and
        i32.const -2139062144
        i32.xor
        i32.and
        i32.eqz
        if $if
          get_local $1
          set_local $0
          br $loop_0
        end ;; $if
      end ;; $loop_0
      get_local $3
      i32.const 255
      i32.and
      if $if_0
        loop $loop_1
          get_local $0
          i32.const 1
          i32.add
          tee_local $0
          i32.load8_s
          br_if $loop_1
        end ;; $loop_1
      end ;; $if_0
    end ;; $block
    get_local $0
    get_local $2
    i32.sub
    )
  
  (func $_malloc (type $5)
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
    get_global $17
    set_local $14
    get_global $17
    i32.const 16
    i32.add
    set_global $17
    get_local $0
    i32.const 245
    i32.lt_u
    if $if (result i32)
      i32.const 1076
      i32.load
      tee_local $7
      i32.const 16
      get_local $0
      i32.const 11
      i32.add
      i32.const -8
      i32.and
      get_local $0
      i32.const 11
      i32.lt_u
      select
      tee_local $2
      i32.const 3
      i32.shr_u
      tee_local $0
      i32.shr_u
      tee_local $3
      i32.const 3
      i32.and
      if $if_0
        get_local $3
        i32.const 1
        i32.and
        i32.const 1
        i32.xor
        get_local $0
        i32.add
        tee_local $1
        i32.const 3
        i32.shl
        i32.const 1116
        i32.add
        tee_local $2
        i32.const 8
        i32.add
        tee_local $4
        i32.load
        tee_local $3
        i32.const 8
        i32.add
        tee_local $6
        i32.load
        set_local $0
        get_local $0
        get_local $2
        i32.eq
        if $if_1
          i32.const 1076
          i32.const 1
          get_local $1
          i32.shl
          i32.const -1
          i32.xor
          get_local $7
          i32.and
          i32.store
        else
          i32.const 1092
          i32.load
          get_local $0
          i32.gt_u
          if $if_2
            call $_abort
          end ;; $if_2
          get_local $3
          get_local $0
          i32.const 12
          i32.add
          tee_local $5
          i32.load
          i32.eq
          if $if_3
            get_local $5
            get_local $2
            i32.store
            get_local $4
            get_local $0
            i32.store
          else
            call $_abort
          end ;; $if_3
        end ;; $if_1
        get_local $3
        get_local $1
        i32.const 3
        i32.shl
        tee_local $0
        i32.const 3
        i32.or
        i32.store offset=4
        get_local $0
        get_local $3
        i32.add
        i32.const 4
        i32.add
        tee_local $0
        get_local $0
        i32.load
        i32.const 1
        i32.or
        i32.store
        get_local $14
        set_global $17
        get_local $6
        return
      end ;; $if_0
      get_local $2
      i32.const 1084
      i32.load
      tee_local $13
      i32.gt_u
      if $if_4 (result i32)
        get_local $3
        if $if_5
          get_local $3
          get_local $0
          i32.shl
          i32.const 2
          get_local $0
          i32.shl
          tee_local $0
          i32.const 0
          get_local $0
          i32.sub
          i32.or
          i32.and
          tee_local $0
          i32.const 0
          get_local $0
          i32.sub
          i32.and
          i32.const -1
          i32.add
          tee_local $0
          i32.const 12
          i32.shr_u
          i32.const 16
          i32.and
          tee_local $3
          get_local $0
          get_local $3
          i32.shr_u
          tee_local $0
          i32.const 5
          i32.shr_u
          i32.const 8
          i32.and
          tee_local $3
          i32.or
          get_local $0
          get_local $3
          i32.shr_u
          tee_local $0
          i32.const 2
          i32.shr_u
          i32.const 4
          i32.and
          tee_local $3
          i32.or
          get_local $0
          get_local $3
          i32.shr_u
          tee_local $0
          i32.const 1
          i32.shr_u
          i32.const 2
          i32.and
          tee_local $3
          i32.or
          get_local $0
          get_local $3
          i32.shr_u
          tee_local $0
          i32.const 1
          i32.shr_u
          i32.const 1
          i32.and
          tee_local $3
          i32.or
          get_local $0
          get_local $3
          i32.shr_u
          i32.add
          tee_local $1
          i32.const 3
          i32.shl
          i32.const 1116
          i32.add
          tee_local $5
          i32.const 8
          i32.add
          tee_local $9
          i32.load
          tee_local $0
          i32.const 8
          i32.add
          tee_local $10
          i32.load
          set_local $3
          get_local $3
          get_local $5
          i32.eq
          if $if_6
            i32.const 1076
            i32.const 1
            get_local $1
            i32.shl
            i32.const -1
            i32.xor
            get_local $7
            i32.and
            tee_local $4
            i32.store
          else
            i32.const 1092
            i32.load
            get_local $3
            i32.gt_u
            if $if_7
              call $_abort
            end ;; $if_7
            get_local $0
            get_local $3
            i32.const 12
            i32.add
            tee_local $11
            i32.load
            i32.eq
            if $if_8
              get_local $11
              get_local $5
              i32.store
              get_local $9
              get_local $3
              i32.store
              get_local $7
              set_local $4
            else
              call $_abort
            end ;; $if_8
          end ;; $if_6
          get_local $0
          get_local $2
          i32.const 3
          i32.or
          i32.store offset=4
          get_local $0
          get_local $2
          i32.add
          tee_local $7
          get_local $1
          i32.const 3
          i32.shl
          tee_local $3
          get_local $2
          i32.sub
          tee_local $5
          i32.const 1
          i32.or
          i32.store offset=4
          get_local $0
          get_local $3
          i32.add
          get_local $5
          i32.store
          get_local $13
          if $if_9
            i32.const 1096
            i32.load
            set_local $2
            get_local $13
            i32.const 3
            i32.shr_u
            tee_local $3
            i32.const 3
            i32.shl
            i32.const 1116
            i32.add
            set_local $0
            i32.const 1
            get_local $3
            i32.shl
            tee_local $3
            get_local $4
            i32.and
            if $if_10
              i32.const 1092
              i32.load
              get_local $0
              i32.const 8
              i32.add
              tee_local $3
              i32.load
              tee_local $1
              i32.gt_u
              if $if_11
                call $_abort
              else
                get_local $1
                set_local $6
                get_local $3
                set_local $12
              end ;; $if_11
            else
              i32.const 1076
              get_local $3
              get_local $4
              i32.or
              i32.store
              get_local $0
              set_local $6
              get_local $0
              i32.const 8
              i32.add
              set_local $12
            end ;; $if_10
            get_local $12
            get_local $2
            i32.store
            get_local $6
            get_local $2
            i32.store offset=12
            get_local $2
            get_local $6
            i32.store offset=8
            get_local $2
            get_local $0
            i32.store offset=12
          end ;; $if_9
          i32.const 1084
          get_local $5
          i32.store
          i32.const 1096
          get_local $7
          i32.store
          get_local $14
          set_global $17
          get_local $10
          return
        end ;; $if_5
        i32.const 1080
        i32.load
        tee_local $12
        if $if_12 (result i32)
          get_local $12
          i32.const 0
          get_local $12
          i32.sub
          i32.and
          i32.const -1
          i32.add
          tee_local $0
          i32.const 12
          i32.shr_u
          i32.const 16
          i32.and
          tee_local $3
          get_local $0
          get_local $3
          i32.shr_u
          tee_local $0
          i32.const 5
          i32.shr_u
          i32.const 8
          i32.and
          tee_local $3
          i32.or
          get_local $0
          get_local $3
          i32.shr_u
          tee_local $0
          i32.const 2
          i32.shr_u
          i32.const 4
          i32.and
          tee_local $3
          i32.or
          get_local $0
          get_local $3
          i32.shr_u
          tee_local $0
          i32.const 1
          i32.shr_u
          i32.const 2
          i32.and
          tee_local $3
          i32.or
          get_local $0
          get_local $3
          i32.shr_u
          tee_local $0
          i32.const 1
          i32.shr_u
          i32.const 1
          i32.and
          tee_local $3
          i32.or
          get_local $0
          get_local $3
          i32.shr_u
          i32.add
          i32.const 2
          i32.shl
          i32.const 1380
          i32.add
          i32.load
          tee_local $4
          set_local $0
          get_local $4
          i32.load offset=4
          i32.const -8
          i32.and
          get_local $2
          i32.sub
          set_local $10
          loop $loop
            block $block
              get_local $0
              i32.load offset=16
              tee_local $3
              if $if_13
                get_local $3
                set_local $0
              else
                get_local $0
                i32.load offset=20
                tee_local $0
                i32.eqz
                br_if $block
              end ;; $if_13
              get_local $0
              get_local $4
              get_local $0
              i32.load offset=4
              i32.const -8
              i32.and
              get_local $2
              i32.sub
              tee_local $3
              get_local $10
              i32.lt_u
              tee_local $6
              select
              set_local $4
              get_local $3
              get_local $10
              get_local $6
              select
              set_local $10
              br $loop
            end ;; $block
          end ;; $loop
          i32.const 1092
          i32.load
          tee_local $15
          get_local $4
          i32.gt_u
          if $if_14
            call $_abort
          end ;; $if_14
          get_local $2
          get_local $4
          i32.add
          tee_local $8
          get_local $4
          i32.le_u
          if $if_15
            call $_abort
          end ;; $if_15
          get_local $4
          i32.load offset=24
          set_local $11
          get_local $4
          get_local $4
          i32.load offset=12
          tee_local $0
          i32.eq
          if $if_16
            block $block_0
              get_local $4
              i32.const 20
              i32.add
              tee_local $3
              i32.load
              tee_local $0
              i32.eqz
              if $if_17
                get_local $4
                i32.const 16
                i32.add
                tee_local $3
                i32.load
                tee_local $0
                i32.eqz
                br_if $block_0
              end ;; $if_17
              loop $loop_0
                block $block_1
                  get_local $0
                  i32.const 20
                  i32.add
                  tee_local $6
                  i32.load
                  tee_local $9
                  i32.eqz
                  if $if_18
                    get_local $0
                    i32.const 16
                    i32.add
                    tee_local $6
                    i32.load
                    tee_local $9
                    i32.eqz
                    br_if $block_1
                  end ;; $if_18
                  get_local $6
                  set_local $3
                  get_local $9
                  set_local $0
                  br $loop_0
                end ;; $block_1
              end ;; $loop_0
              get_local $15
              get_local $3
              i32.gt_u
              if $if_19
                call $_abort
              else
                get_local $3
                i32.const 0
                i32.store
                get_local $0
                set_local $1
              end ;; $if_19
            end ;; $block_0
          else
            get_local $15
            get_local $4
            i32.load offset=8
            tee_local $3
            i32.gt_u
            if $if_20
              call $_abort
            end ;; $if_20
            get_local $3
            i32.const 12
            i32.add
            tee_local $6
            i32.load
            get_local $4
            i32.ne
            if $if_21
              call $_abort
            end ;; $if_21
            get_local $4
            get_local $0
            i32.const 8
            i32.add
            tee_local $9
            i32.load
            i32.eq
            if $if_22
              get_local $6
              get_local $0
              i32.store
              get_local $9
              get_local $3
              i32.store
              get_local $0
              set_local $1
            else
              call $_abort
            end ;; $if_22
          end ;; $if_16
          get_local $11
          if $if_23
            block $block_2
              get_local $4
              get_local $4
              i32.load offset=28
              tee_local $0
              i32.const 2
              i32.shl
              i32.const 1380
              i32.add
              tee_local $3
              i32.load
              i32.eq
              if $if_24
                get_local $3
                get_local $1
                i32.store
                get_local $1
                i32.eqz
                if $if_25
                  i32.const 1080
                  get_local $12
                  i32.const 1
                  get_local $0
                  i32.shl
                  i32.const -1
                  i32.xor
                  i32.and
                  i32.store
                  br $block_2
                end ;; $if_25
              else
                i32.const 1092
                i32.load
                get_local $11
                i32.gt_u
                if $if_26
                  call $_abort
                else
                  get_local $11
                  i32.const 16
                  i32.add
                  tee_local $0
                  get_local $11
                  i32.const 20
                  i32.add
                  get_local $4
                  get_local $0
                  i32.load
                  i32.eq
                  select
                  get_local $1
                  i32.store
                  get_local $1
                  i32.eqz
                  br_if $block_2
                end ;; $if_26
              end ;; $if_24
              i32.const 1092
              i32.load
              tee_local $3
              get_local $1
              i32.gt_u
              if $if_27
                call $_abort
              end ;; $if_27
              get_local $1
              get_local $11
              i32.store offset=24
              get_local $4
              i32.load offset=16
              tee_local $0
              if $if_28
                get_local $3
                get_local $0
                i32.gt_u
                if $if_29
                  call $_abort
                else
                  get_local $1
                  get_local $0
                  i32.store offset=16
                  get_local $0
                  get_local $1
                  i32.store offset=24
                end ;; $if_29
              end ;; $if_28
              get_local $4
              i32.load offset=20
              tee_local $0
              if $if_30
                i32.const 1092
                i32.load
                get_local $0
                i32.gt_u
                if $if_31
                  call $_abort
                else
                  get_local $1
                  get_local $0
                  i32.store offset=20
                  get_local $0
                  get_local $1
                  i32.store offset=24
                end ;; $if_31
              end ;; $if_30
            end ;; $block_2
          end ;; $if_23
          get_local $10
          i32.const 16
          i32.lt_u
          if $if_32
            get_local $4
            get_local $2
            get_local $10
            i32.add
            tee_local $0
            i32.const 3
            i32.or
            i32.store offset=4
            get_local $0
            get_local $4
            i32.add
            i32.const 4
            i32.add
            tee_local $0
            get_local $0
            i32.load
            i32.const 1
            i32.or
            i32.store
          else
            get_local $4
            get_local $2
            i32.const 3
            i32.or
            i32.store offset=4
            get_local $8
            get_local $10
            i32.const 1
            i32.or
            i32.store offset=4
            get_local $8
            get_local $10
            i32.add
            get_local $10
            i32.store
            get_local $13
            if $if_33
              i32.const 1096
              i32.load
              set_local $2
              get_local $13
              i32.const 3
              i32.shr_u
              tee_local $3
              i32.const 3
              i32.shl
              i32.const 1116
              i32.add
              set_local $0
              i32.const 1
              get_local $3
              i32.shl
              tee_local $3
              get_local $7
              i32.and
              if $if_34
                i32.const 1092
                i32.load
                get_local $0
                i32.const 8
                i32.add
                tee_local $3
                i32.load
                tee_local $1
                i32.gt_u
                if $if_35
                  call $_abort
                else
                  get_local $1
                  set_local $5
                  get_local $3
                  set_local $16
                end ;; $if_35
              else
                i32.const 1076
                get_local $3
                get_local $7
                i32.or
                i32.store
                get_local $0
                set_local $5
                get_local $0
                i32.const 8
                i32.add
                set_local $16
              end ;; $if_34
              get_local $16
              get_local $2
              i32.store
              get_local $5
              get_local $2
              i32.store offset=12
              get_local $2
              get_local $5
              i32.store offset=8
              get_local $2
              get_local $0
              i32.store offset=12
            end ;; $if_33
            i32.const 1084
            get_local $10
            i32.store
            i32.const 1096
            get_local $8
            i32.store
          end ;; $if_32
          get_local $14
          set_global $17
          get_local $4
          i32.const 8
          i32.add
          return
        else
          get_local $2
        end ;; $if_12
      else
        get_local $2
      end ;; $if_4
    else
      get_local $0
      i32.const -65
      i32.gt_u
      if $if_36 (result i32)
        i32.const -1
      else
        block $block_3 (result i32)
          get_local $0
          i32.const 11
          i32.add
          tee_local $0
          i32.const -8
          i32.and
          set_local $4
          i32.const 1080
          i32.load
          tee_local $6
          if $if_37 (result i32)
            get_local $0
            i32.const 8
            i32.shr_u
            tee_local $0
            if $if_38 (result i32)
              get_local $4
              i32.const 16777215
              i32.gt_u
              if $if_39 (result i32)
                i32.const 31
              else
                get_local $0
                get_local $0
                i32.const 1048320
                i32.add
                i32.const 16
                i32.shr_u
                i32.const 8
                i32.and
                tee_local $1
                i32.shl
                tee_local $2
                i32.const 520192
                i32.add
                i32.const 16
                i32.shr_u
                i32.const 4
                i32.and
                set_local $0
                i32.const 14
                get_local $0
                get_local $1
                i32.or
                get_local $2
                get_local $0
                i32.shl
                tee_local $0
                i32.const 245760
                i32.add
                i32.const 16
                i32.shr_u
                i32.const 2
                i32.and
                tee_local $1
                i32.or
                i32.sub
                get_local $0
                get_local $1
                i32.shl
                i32.const 15
                i32.shr_u
                i32.add
                tee_local $0
                i32.const 1
                i32.shl
                get_local $4
                get_local $0
                i32.const 7
                i32.add
                i32.shr_u
                i32.const 1
                i32.and
                i32.or
              end ;; $if_39
            else
              i32.const 0
            end ;; $if_38
            set_local $18
            i32.const 0
            get_local $4
            i32.sub
            set_local $2
            block $block_4
              block $block_5
                get_local $18
                i32.const 2
                i32.shl
                i32.const 1380
                i32.add
                i32.load
                tee_local $0
                if $if_40 (result i32)
                  i32.const 0
                  set_local $1
                  get_local $4
                  i32.const 0
                  i32.const 25
                  get_local $18
                  i32.const 1
                  i32.shr_u
                  i32.sub
                  get_local $18
                  i32.const 31
                  i32.eq
                  select
                  i32.shl
                  set_local $12
                  loop $loop_1 (result i32)
                    get_local $0
                    i32.load offset=4
                    i32.const -8
                    i32.and
                    get_local $4
                    i32.sub
                    tee_local $16
                    get_local $2
                    i32.lt_u
                    if $if_41
                      get_local $16
                      if $if_42 (result i32)
                        get_local $16
                        set_local $2
                        get_local $0
                      else
                        i32.const 0
                        set_local $2
                        get_local $0
                        set_local $1
                        br $block_5
                      end ;; $if_42
                      set_local $1
                    end ;; $if_41
                    get_local $5
                    get_local $0
                    i32.load offset=20
                    tee_local $5
                    get_local $5
                    i32.eqz
                    get_local $5
                    get_local $0
                    i32.const 16
                    i32.add
                    get_local $12
                    i32.const 31
                    i32.shr_u
                    i32.const 2
                    i32.shl
                    i32.add
                    i32.load
                    tee_local $0
                    i32.eq
                    i32.or
                    select
                    set_local $5
                    get_local $12
                    i32.const 1
                    i32.shl
                    set_local $12
                    get_local $0
                    br_if $loop_1
                    get_local $1
                  end ;; $loop_1
                else
                  i32.const 0
                end ;; $if_40
                set_local $0
                get_local $0
                get_local $5
                i32.or
                if $if_43 (result i32)
                  get_local $5
                else
                  get_local $4
                  get_local $6
                  i32.const 2
                  get_local $18
                  i32.shl
                  tee_local $0
                  i32.const 0
                  get_local $0
                  i32.sub
                  i32.or
                  i32.and
                  tee_local $1
                  i32.eqz
                  br_if $block_3
                  drop
                  i32.const 0
                  set_local $0
                  get_local $1
                  i32.const 0
                  get_local $1
                  i32.sub
                  i32.and
                  i32.const -1
                  i32.add
                  tee_local $1
                  i32.const 12
                  i32.shr_u
                  i32.const 16
                  i32.and
                  tee_local $5
                  get_local $1
                  get_local $5
                  i32.shr_u
                  tee_local $1
                  i32.const 5
                  i32.shr_u
                  i32.const 8
                  i32.and
                  tee_local $5
                  i32.or
                  get_local $1
                  get_local $5
                  i32.shr_u
                  tee_local $1
                  i32.const 2
                  i32.shr_u
                  i32.const 4
                  i32.and
                  tee_local $5
                  i32.or
                  get_local $1
                  get_local $5
                  i32.shr_u
                  tee_local $1
                  i32.const 1
                  i32.shr_u
                  i32.const 2
                  i32.and
                  tee_local $5
                  i32.or
                  get_local $1
                  get_local $5
                  i32.shr_u
                  tee_local $1
                  i32.const 1
                  i32.shr_u
                  i32.const 1
                  i32.and
                  tee_local $5
                  i32.or
                  get_local $1
                  get_local $5
                  i32.shr_u
                  i32.add
                  i32.const 2
                  i32.shl
                  i32.const 1380
                  i32.add
                  i32.load
                end ;; $if_43
                tee_local $1
                br_if $block_5
                get_local $0
                set_local $5
                br $block_4
              end ;; $block_5
              get_local $0
              set_local $5
              get_local $2
              set_local $0
              loop $loop_2 (result i32)
                get_local $1
                i32.load offset=4
                set_local $12
                get_local $1
                i32.load offset=16
                tee_local $2
                i32.eqz
                if $if_44
                  get_local $1
                  i32.load offset=20
                  set_local $2
                end ;; $if_44
                get_local $12
                i32.const -8
                i32.and
                get_local $4
                i32.sub
                tee_local $16
                get_local $0
                i32.lt_u
                set_local $12
                get_local $16
                get_local $0
                get_local $12
                select
                set_local $0
                get_local $1
                get_local $5
                get_local $12
                select
                set_local $5
                get_local $2
                if $if_45 (result i32)
                  get_local $2
                  set_local $1
                  br $loop_2
                else
                  get_local $0
                end ;; $if_45
              end ;; $loop_2
              set_local $2
            end ;; $block_4
            get_local $5
            if $if_46 (result i32)
              get_local $2
              i32.const 1084
              i32.load
              get_local $4
              i32.sub
              i32.lt_u
              if $if_47 (result i32)
                i32.const 1092
                i32.load
                tee_local $17
                get_local $5
                i32.gt_u
                if $if_48
                  call $_abort
                end ;; $if_48
                get_local $4
                get_local $5
                i32.add
                tee_local $8
                get_local $5
                i32.le_u
                if $if_49
                  call $_abort
                end ;; $if_49
                get_local $5
                i32.load offset=24
                set_local $15
                get_local $5
                get_local $5
                i32.load offset=12
                tee_local $0
                i32.eq
                if $if_50
                  block $block_6
                    get_local $5
                    i32.const 20
                    i32.add
                    tee_local $1
                    i32.load
                    tee_local $0
                    i32.eqz
                    if $if_51
                      get_local $5
                      i32.const 16
                      i32.add
                      tee_local $1
                      i32.load
                      tee_local $0
                      i32.eqz
                      br_if $block_6
                    end ;; $if_51
                    loop $loop_3
                      block $block_7
                        get_local $0
                        i32.const 20
                        i32.add
                        tee_local $9
                        i32.load
                        tee_local $11
                        i32.eqz
                        if $if_52
                          get_local $0
                          i32.const 16
                          i32.add
                          tee_local $9
                          i32.load
                          tee_local $11
                          i32.eqz
                          br_if $block_7
                        end ;; $if_52
                        get_local $9
                        set_local $1
                        get_local $11
                        set_local $0
                        br $loop_3
                      end ;; $block_7
                    end ;; $loop_3
                    get_local $17
                    get_local $1
                    i32.gt_u
                    if $if_53
                      call $_abort
                    else
                      get_local $1
                      i32.const 0
                      i32.store
                      get_local $0
                      set_local $7
                    end ;; $if_53
                  end ;; $block_6
                else
                  get_local $17
                  get_local $5
                  i32.load offset=8
                  tee_local $1
                  i32.gt_u
                  if $if_54
                    call $_abort
                  end ;; $if_54
                  get_local $1
                  i32.const 12
                  i32.add
                  tee_local $9
                  i32.load
                  get_local $5
                  i32.ne
                  if $if_55
                    call $_abort
                  end ;; $if_55
                  get_local $5
                  get_local $0
                  i32.const 8
                  i32.add
                  tee_local $11
                  i32.load
                  i32.eq
                  if $if_56
                    get_local $9
                    get_local $0
                    i32.store
                    get_local $11
                    get_local $1
                    i32.store
                    get_local $0
                    set_local $7
                  else
                    call $_abort
                  end ;; $if_56
                end ;; $if_50
                get_local $15
                if $if_57
                  block $block_8
                    get_local $5
                    get_local $5
                    i32.load offset=28
                    tee_local $0
                    i32.const 2
                    i32.shl
                    i32.const 1380
                    i32.add
                    tee_local $1
                    i32.load
                    i32.eq
                    if $if_58
                      get_local $1
                      get_local $7
                      i32.store
                      get_local $7
                      i32.eqz
                      if $if_59
                        i32.const 1080
                        get_local $6
                        i32.const 1
                        get_local $0
                        i32.shl
                        i32.const -1
                        i32.xor
                        i32.and
                        tee_local $3
                        i32.store
                        br $block_8
                      end ;; $if_59
                    else
                      i32.const 1092
                      i32.load
                      get_local $15
                      i32.gt_u
                      if $if_60
                        call $_abort
                      else
                        get_local $15
                        i32.const 16
                        i32.add
                        tee_local $0
                        get_local $15
                        i32.const 20
                        i32.add
                        get_local $5
                        get_local $0
                        i32.load
                        i32.eq
                        select
                        get_local $7
                        i32.store
                        get_local $7
                        i32.eqz
                        if $if_61
                          get_local $6
                          set_local $3
                          br $block_8
                        end ;; $if_61
                      end ;; $if_60
                    end ;; $if_58
                    i32.const 1092
                    i32.load
                    tee_local $1
                    get_local $7
                    i32.gt_u
                    if $if_62
                      call $_abort
                    end ;; $if_62
                    get_local $7
                    get_local $15
                    i32.store offset=24
                    get_local $5
                    i32.load offset=16
                    tee_local $0
                    if $if_63
                      get_local $1
                      get_local $0
                      i32.gt_u
                      if $if_64
                        call $_abort
                      else
                        get_local $7
                        get_local $0
                        i32.store offset=16
                        get_local $0
                        get_local $7
                        i32.store offset=24
                      end ;; $if_64
                    end ;; $if_63
                    get_local $5
                    i32.load offset=20
                    tee_local $0
                    if $if_65
                      i32.const 1092
                      i32.load
                      get_local $0
                      i32.gt_u
                      if $if_66
                        call $_abort
                      else
                        get_local $7
                        get_local $0
                        i32.store offset=20
                        get_local $0
                        get_local $7
                        i32.store offset=24
                        get_local $6
                        set_local $3
                      end ;; $if_66
                    else
                      get_local $6
                      set_local $3
                    end ;; $if_65
                  end ;; $block_8
                else
                  get_local $6
                  set_local $3
                end ;; $if_57
                get_local $2
                i32.const 16
                i32.lt_u
                if $if_67
                  get_local $5
                  get_local $2
                  get_local $4
                  i32.add
                  tee_local $0
                  i32.const 3
                  i32.or
                  i32.store offset=4
                  get_local $0
                  get_local $5
                  i32.add
                  i32.const 4
                  i32.add
                  tee_local $0
                  get_local $0
                  i32.load
                  i32.const 1
                  i32.or
                  i32.store
                else
                  block $block_9
                    get_local $5
                    get_local $4
                    i32.const 3
                    i32.or
                    i32.store offset=4
                    get_local $8
                    get_local $2
                    i32.const 1
                    i32.or
                    i32.store offset=4
                    get_local $2
                    get_local $8
                    i32.add
                    get_local $2
                    i32.store
                    get_local $2
                    i32.const 3
                    i32.shr_u
                    set_local $1
                    get_local $2
                    i32.const 256
                    i32.lt_u
                    if $if_68
                      get_local $1
                      i32.const 3
                      i32.shl
                      i32.const 1116
                      i32.add
                      set_local $0
                      i32.const 1076
                      i32.load
                      tee_local $3
                      i32.const 1
                      get_local $1
                      i32.shl
                      tee_local $1
                      i32.and
                      if $if_69
                        i32.const 1092
                        i32.load
                        get_local $0
                        i32.const 8
                        i32.add
                        tee_local $3
                        i32.load
                        tee_local $1
                        i32.gt_u
                        if $if_70
                          call $_abort
                        else
                          get_local $1
                          set_local $13
                          get_local $3
                          set_local $19
                        end ;; $if_70
                      else
                        i32.const 1076
                        get_local $1
                        get_local $3
                        i32.or
                        i32.store
                        get_local $0
                        set_local $13
                        get_local $0
                        i32.const 8
                        i32.add
                        set_local $19
                      end ;; $if_69
                      get_local $19
                      get_local $8
                      i32.store
                      get_local $13
                      get_local $8
                      i32.store offset=12
                      get_local $8
                      get_local $13
                      i32.store offset=8
                      get_local $8
                      get_local $0
                      i32.store offset=12
                      br $block_9
                    end ;; $if_68
                    get_local $2
                    i32.const 8
                    i32.shr_u
                    tee_local $0
                    if $if_71 (result i32)
                      get_local $2
                      i32.const 16777215
                      i32.gt_u
                      if $if_72 (result i32)
                        i32.const 31
                      else
                        get_local $0
                        get_local $0
                        i32.const 1048320
                        i32.add
                        i32.const 16
                        i32.shr_u
                        i32.const 8
                        i32.and
                        tee_local $1
                        i32.shl
                        tee_local $4
                        i32.const 520192
                        i32.add
                        i32.const 16
                        i32.shr_u
                        i32.const 4
                        i32.and
                        set_local $0
                        i32.const 14
                        get_local $0
                        get_local $1
                        i32.or
                        get_local $4
                        get_local $0
                        i32.shl
                        tee_local $0
                        i32.const 245760
                        i32.add
                        i32.const 16
                        i32.shr_u
                        i32.const 2
                        i32.and
                        tee_local $1
                        i32.or
                        i32.sub
                        get_local $0
                        get_local $1
                        i32.shl
                        i32.const 15
                        i32.shr_u
                        i32.add
                        tee_local $0
                        i32.const 1
                        i32.shl
                        get_local $2
                        get_local $0
                        i32.const 7
                        i32.add
                        i32.shr_u
                        i32.const 1
                        i32.and
                        i32.or
                      end ;; $if_72
                    else
                      i32.const 0
                    end ;; $if_71
                    tee_local $1
                    i32.const 2
                    i32.shl
                    i32.const 1380
                    i32.add
                    set_local $0
                    get_local $8
                    get_local $1
                    i32.store offset=28
                    get_local $8
                    i32.const 16
                    i32.add
                    tee_local $4
                    i32.const 0
                    i32.store offset=4
                    get_local $4
                    i32.const 0
                    i32.store
                    i32.const 1
                    get_local $1
                    i32.shl
                    tee_local $4
                    get_local $3
                    i32.and
                    i32.eqz
                    if $if_73
                      i32.const 1080
                      get_local $3
                      get_local $4
                      i32.or
                      i32.store
                      get_local $0
                      get_local $8
                      i32.store
                      get_local $8
                      get_local $0
                      i32.store offset=24
                      get_local $8
                      get_local $8
                      i32.store offset=12
                      get_local $8
                      get_local $8
                      i32.store offset=8
                      br $block_9
                    end ;; $if_73
                    get_local $2
                    get_local $0
                    i32.load
                    tee_local $0
                    i32.load offset=4
                    i32.const -8
                    i32.and
                    i32.eq
                    if $if_74
                      get_local $0
                      set_local $10
                    else
                      block $block_10
                        get_local $2
                        i32.const 0
                        i32.const 25
                        get_local $1
                        i32.const 1
                        i32.shr_u
                        i32.sub
                        get_local $1
                        i32.const 31
                        i32.eq
                        select
                        i32.shl
                        set_local $1
                        loop $loop_4
                          get_local $0
                          i32.const 16
                          i32.add
                          get_local $1
                          i32.const 31
                          i32.shr_u
                          i32.const 2
                          i32.shl
                          i32.add
                          tee_local $4
                          i32.load
                          tee_local $3
                          if $if_75
                            get_local $1
                            i32.const 1
                            i32.shl
                            set_local $1
                            get_local $2
                            get_local $3
                            i32.load offset=4
                            i32.const -8
                            i32.and
                            i32.eq
                            if $if_76
                              get_local $3
                              set_local $10
                              br $block_10
                            else
                              get_local $3
                              set_local $0
                              br $loop_4
                            end ;; $if_76
                            unreachable
                          end ;; $if_75
                        end ;; $loop_4
                        i32.const 1092
                        i32.load
                        get_local $4
                        i32.gt_u
                        if $if_77
                          call $_abort
                        else
                          get_local $4
                          get_local $8
                          i32.store
                          get_local $8
                          get_local $0
                          i32.store offset=24
                          get_local $8
                          get_local $8
                          i32.store offset=12
                          get_local $8
                          get_local $8
                          i32.store offset=8
                          br $block_9
                        end ;; $if_77
                      end ;; $block_10
                    end ;; $if_74
                    i32.const 1092
                    i32.load
                    tee_local $0
                    get_local $10
                    i32.le_u
                    get_local $0
                    get_local $10
                    i32.const 8
                    i32.add
                    tee_local $3
                    i32.load
                    tee_local $0
                    i32.le_u
                    i32.and
                    if $if_78
                      get_local $0
                      get_local $8
                      i32.store offset=12
                      get_local $3
                      get_local $8
                      i32.store
                      get_local $8
                      get_local $0
                      i32.store offset=8
                      get_local $8
                      get_local $10
                      i32.store offset=12
                      get_local $8
                      i32.const 0
                      i32.store offset=24
                    else
                      call $_abort
                    end ;; $if_78
                  end ;; $block_9
                end ;; $if_67
                get_local $14
                set_global $17
                get_local $5
                i32.const 8
                i32.add
                return
              else
                get_local $4
              end ;; $if_47
            else
              get_local $4
            end ;; $if_46
          else
            get_local $4
          end ;; $if_37
        end ;; $block_3
      end ;; $if_36
    end ;; $if
    set_local $3
    block $block_11
      i32.const 1084
      i32.load
      tee_local $1
      get_local $3
      i32.ge_u
      if $if_79
        i32.const 1096
        i32.load
        set_local $0
        get_local $1
        get_local $3
        i32.sub
        tee_local $2
        i32.const 15
        i32.gt_u
        if $if_80
          i32.const 1096
          get_local $0
          get_local $3
          i32.add
          tee_local $4
          i32.store
          i32.const 1084
          get_local $2
          i32.store
          get_local $4
          get_local $2
          i32.const 1
          i32.or
          i32.store offset=4
          get_local $0
          get_local $1
          i32.add
          get_local $2
          i32.store
          get_local $0
          get_local $3
          i32.const 3
          i32.or
          i32.store offset=4
        else
          i32.const 1084
          i32.const 0
          i32.store
          i32.const 1096
          i32.const 0
          i32.store
          get_local $0
          get_local $1
          i32.const 3
          i32.or
          i32.store offset=4
          get_local $0
          get_local $1
          i32.add
          i32.const 4
          i32.add
          tee_local $3
          get_local $3
          i32.load
          i32.const 1
          i32.or
          i32.store
        end ;; $if_80
        br $block_11
      end ;; $if_79
      block $block_12
        i32.const 1088
        i32.load
        tee_local $1
        get_local $3
        i32.gt_u
        if $if_81
          i32.const 1088
          get_local $1
          get_local $3
          i32.sub
          tee_local $1
          i32.store
          br $block_12
        end ;; $if_81
        get_local $3
        i32.const 47
        i32.add
        tee_local $6
        i32.const 1548
        i32.load
        if $if_82 (result i32)
          i32.const 1556
          i32.load
        else
          i32.const 1556
          i32.const 4096
          i32.store
          i32.const 1552
          i32.const 4096
          i32.store
          i32.const 1560
          i32.const -1
          i32.store
          i32.const 1564
          i32.const -1
          i32.store
          i32.const 1568
          i32.const 0
          i32.store
          i32.const 1520
          i32.const 0
          i32.store
          i32.const 1548
          get_local $14
          i32.const -16
          i32.and
          i32.const 1431655768
          i32.xor
          i32.store
          i32.const 4096
        end ;; $if_82
        tee_local $0
        i32.add
        tee_local $5
        i32.const 0
        get_local $0
        i32.sub
        tee_local $7
        i32.and
        tee_local $4
        get_local $3
        i32.le_u
        if $if_83
          get_local $14
          set_global $17
          i32.const 0
          return
        end ;; $if_83
        i32.const 1516
        i32.load
        tee_local $0
        if $if_84
          get_local $4
          i32.const 1508
          i32.load
          tee_local $2
          i32.add
          tee_local $10
          get_local $2
          i32.le_u
          get_local $10
          get_local $0
          i32.gt_u
          i32.or
          if $if_85
            get_local $14
            set_global $17
            i32.const 0
            return
          end ;; $if_85
        end ;; $if_84
        get_local $3
        i32.const 48
        i32.add
        set_local $10
        block $block_13
          block $block_14
            i32.const 1520
            i32.load
            i32.const 4
            i32.and
            if $if_86
              i32.const 0
              set_local $1
            else
              block $block_15
                block $block_16
                  block $block_17
                    i32.const 1100
                    i32.load
                    tee_local $0
                    i32.eqz
                    br_if $block_17
                    i32.const 1524
                    set_local $2
                    loop $loop_5
                      block $block_18
                        get_local $2
                        i32.load
                        tee_local $13
                        get_local $0
                        i32.le_u
                        if $if_87
                          get_local $13
                          get_local $2
                          i32.load offset=4
                          i32.add
                          get_local $0
                          i32.gt_u
                          br_if $block_18
                        end ;; $if_87
                        get_local $2
                        i32.load offset=8
                        tee_local $2
                        br_if $loop_5
                        br $block_17
                      end ;; $block_18
                    end ;; $loop_5
                    get_local $7
                    get_local $5
                    get_local $1
                    i32.sub
                    i32.and
                    tee_local $1
                    i32.const 2147483647
                    i32.lt_u
                    if $if_88
                      get_local $2
                      i32.const 4
                      i32.add
                      set_local $5
                      get_local $1
                      call $_sbrk
                      tee_local $0
                      get_local $2
                      i32.load
                      get_local $5
                      i32.load
                      i32.add
                      i32.ne
                      br_if $block_16
                      get_local $0
                      i32.const -1
                      i32.ne
                      br_if $block_14
                    else
                      i32.const 0
                      set_local $1
                    end ;; $if_88
                    br $block_15
                  end ;; $block_17
                  i32.const 0
                  call $_sbrk
                  tee_local $0
                  i32.const -1
                  i32.eq
                  if $if_89 (result i32)
                    i32.const 0
                  else
                    i32.const 1508
                    i32.load
                    tee_local $5
                    get_local $4
                    get_local $0
                    i32.const 1552
                    i32.load
                    tee_local $1
                    i32.const -1
                    i32.add
                    tee_local $2
                    i32.add
                    i32.const 0
                    get_local $1
                    i32.sub
                    i32.and
                    get_local $0
                    i32.sub
                    i32.const 0
                    get_local $0
                    get_local $2
                    i32.and
                    select
                    i32.add
                    tee_local $1
                    i32.add
                    set_local $2
                    get_local $1
                    i32.const 2147483647
                    i32.lt_u
                    get_local $1
                    get_local $3
                    i32.gt_u
                    i32.and
                    if $if_90 (result i32)
                      i32.const 1516
                      i32.load
                      tee_local $7
                      if $if_91
                        get_local $2
                        get_local $5
                        i32.le_u
                        get_local $2
                        get_local $7
                        i32.gt_u
                        i32.or
                        if $if_92
                          i32.const 0
                          set_local $1
                          br $block_15
                        end ;; $if_92
                      end ;; $if_91
                      get_local $0
                      get_local $1
                      call $_sbrk
                      tee_local $2
                      i32.eq
                      br_if $block_14
                      get_local $2
                      set_local $0
                      br $block_16
                    else
                      i32.const 0
                    end ;; $if_90
                  end ;; $if_89
                  set_local $1
                  br $block_15
                end ;; $block_16
                get_local $0
                i32.const -1
                i32.ne
                get_local $1
                i32.const 2147483647
                i32.lt_u
                i32.and
                get_local $10
                get_local $1
                i32.gt_u
                i32.and
                i32.eqz
                if $if_93
                  get_local $0
                  i32.const -1
                  i32.eq
                  if $if_94
                    i32.const 0
                    set_local $1
                    br $block_15
                  else
                    br $block_14
                  end ;; $if_94
                  unreachable
                end ;; $if_93
                i32.const 1556
                i32.load
                tee_local $2
                get_local $6
                get_local $1
                i32.sub
                i32.add
                i32.const 0
                get_local $2
                i32.sub
                i32.and
                tee_local $2
                i32.const 2147483647
                i32.ge_u
                br_if $block_14
                i32.const 0
                get_local $1
                i32.sub
                set_local $6
                get_local $2
                call $_sbrk
                i32.const -1
                i32.eq
                if $if_95 (result i32)
                  get_local $6
                  call $_sbrk
                  drop
                  i32.const 0
                else
                  get_local $1
                  get_local $2
                  i32.add
                  set_local $1
                  br $block_14
                end ;; $if_95
                set_local $1
              end ;; $block_15
              i32.const 1520
              i32.const 1520
              i32.load
              i32.const 4
              i32.or
              i32.store
            end ;; $if_86
            get_local $4
            i32.const 2147483647
            i32.lt_u
            if $if_96
              get_local $4
              call $_sbrk
              set_local $0
              i32.const 0
              call $_sbrk
              tee_local $2
              get_local $0
              i32.sub
              tee_local $6
              get_local $3
              i32.const 40
              i32.add
              i32.gt_u
              set_local $4
              get_local $6
              get_local $1
              get_local $4
              select
              set_local $1
              get_local $4
              i32.const 1
              i32.xor
              get_local $0
              i32.const -1
              i32.eq
              i32.or
              get_local $0
              i32.const -1
              i32.ne
              get_local $2
              i32.const -1
              i32.ne
              i32.and
              get_local $0
              get_local $2
              i32.lt_u
              i32.and
              i32.const 1
              i32.xor
              i32.or
              i32.eqz
              br_if $block_14
            end ;; $if_96
            br $block_13
          end ;; $block_14
          i32.const 1508
          get_local $1
          i32.const 1508
          i32.load
          i32.add
          tee_local $2
          i32.store
          get_local $2
          i32.const 1512
          i32.load
          i32.gt_u
          if $if_97
            i32.const 1512
            get_local $2
            i32.store
          end ;; $if_97
          i32.const 1100
          i32.load
          tee_local $6
          if $if_98
            block $block_19
              i32.const 1524
              set_local $2
              block $block_20
                block $block_21
                  loop $loop_6
                    get_local $0
                    get_local $2
                    i32.load
                    tee_local $4
                    get_local $2
                    i32.load offset=4
                    tee_local $5
                    i32.add
                    i32.eq
                    br_if $block_21
                    get_local $2
                    i32.load offset=8
                    tee_local $2
                    br_if $loop_6
                  end ;; $loop_6
                  br $block_20
                end ;; $block_21
                get_local $2
                i32.const 4
                i32.add
                set_local $7
                get_local $2
                i32.load offset=12
                i32.const 8
                i32.and
                i32.eqz
                if $if_99
                  get_local $4
                  get_local $6
                  i32.le_u
                  get_local $0
                  get_local $6
                  i32.gt_u
                  i32.and
                  if $if_100
                    get_local $7
                    get_local $1
                    get_local $5
                    i32.add
                    i32.store
                    get_local $6
                    i32.const 0
                    get_local $6
                    i32.const 8
                    i32.add
                    tee_local $0
                    i32.sub
                    i32.const 7
                    i32.and
                    i32.const 0
                    get_local $0
                    i32.const 7
                    i32.and
                    select
                    tee_local $2
                    i32.add
                    set_local $0
                    get_local $1
                    i32.const 1088
                    i32.load
                    i32.add
                    tee_local $4
                    get_local $2
                    i32.sub
                    set_local $1
                    i32.const 1100
                    get_local $0
                    i32.store
                    i32.const 1088
                    get_local $1
                    i32.store
                    get_local $0
                    get_local $1
                    i32.const 1
                    i32.or
                    i32.store offset=4
                    get_local $4
                    get_local $6
                    i32.add
                    i32.const 40
                    i32.store offset=4
                    i32.const 1104
                    i32.const 1564
                    i32.load
                    i32.store
                    br $block_19
                  end ;; $if_100
                end ;; $if_99
              end ;; $block_20
              get_local $0
              i32.const 1092
              i32.load
              tee_local $2
              i32.lt_u
              if $if_101
                i32.const 1092
                get_local $0
                i32.store
                get_local $0
                set_local $2
              end ;; $if_101
              get_local $0
              get_local $1
              i32.add
              set_local $5
              i32.const 1524
              set_local $4
              block $block_22
                block $block_23
                  loop $loop_7
                    get_local $5
                    get_local $4
                    i32.load
                    i32.eq
                    br_if $block_23
                    get_local $4
                    i32.load offset=8
                    tee_local $4
                    br_if $loop_7
                  end ;; $loop_7
                  br $block_22
                end ;; $block_23
                get_local $4
                i32.load offset=12
                i32.const 8
                i32.and
                i32.eqz
                if $if_102
                  get_local $4
                  get_local $0
                  i32.store
                  get_local $4
                  i32.const 4
                  i32.add
                  tee_local $4
                  get_local $1
                  get_local $4
                  i32.load
                  i32.add
                  i32.store
                  get_local $3
                  get_local $0
                  i32.const 0
                  get_local $0
                  i32.const 8
                  i32.add
                  tee_local $0
                  i32.sub
                  i32.const 7
                  i32.and
                  i32.const 0
                  get_local $0
                  i32.const 7
                  i32.and
                  select
                  i32.add
                  tee_local $8
                  i32.add
                  set_local $7
                  get_local $5
                  i32.const 0
                  get_local $5
                  i32.const 8
                  i32.add
                  tee_local $0
                  i32.sub
                  i32.const 7
                  i32.and
                  i32.const 0
                  get_local $0
                  i32.const 7
                  i32.and
                  select
                  i32.add
                  tee_local $1
                  get_local $8
                  i32.sub
                  get_local $3
                  i32.sub
                  set_local $4
                  get_local $8
                  get_local $3
                  i32.const 3
                  i32.or
                  i32.store offset=4
                  get_local $1
                  get_local $6
                  i32.eq
                  if $if_103
                    i32.const 1088
                    get_local $4
                    i32.const 1088
                    i32.load
                    i32.add
                    tee_local $0
                    i32.store
                    i32.const 1100
                    get_local $7
                    i32.store
                    get_local $7
                    get_local $0
                    i32.const 1
                    i32.or
                    i32.store offset=4
                  else
                    block $block_24
                      get_local $1
                      i32.const 1096
                      i32.load
                      i32.eq
                      if $if_104
                        i32.const 1084
                        get_local $4
                        i32.const 1084
                        i32.load
                        i32.add
                        tee_local $0
                        i32.store
                        i32.const 1096
                        get_local $7
                        i32.store
                        get_local $7
                        get_local $0
                        i32.const 1
                        i32.or
                        i32.store offset=4
                        get_local $0
                        get_local $7
                        i32.add
                        get_local $0
                        i32.store
                        br $block_24
                      end ;; $if_104
                      get_local $1
                      i32.load offset=4
                      tee_local $0
                      i32.const 3
                      i32.and
                      i32.const 1
                      i32.eq
                      if $if_105 (result i32)
                        get_local $0
                        i32.const -8
                        i32.and
                        set_local $13
                        get_local $0
                        i32.const 3
                        i32.shr_u
                        set_local $5
                        block $block_25
                          get_local $0
                          i32.const 256
                          i32.lt_u
                          if $if_106
                            get_local $1
                            i32.load offset=12
                            set_local $3
                            get_local $1
                            i32.load offset=8
                            tee_local $6
                            get_local $5
                            i32.const 3
                            i32.shl
                            i32.const 1116
                            i32.add
                            tee_local $0
                            i32.ne
                            if $if_107
                              block $block_26
                                get_local $2
                                get_local $6
                                i32.gt_u
                                if $if_108
                                  call $_abort
                                end ;; $if_108
                                get_local $1
                                get_local $6
                                i32.load offset=12
                                i32.eq
                                br_if $block_26
                                call $_abort
                              end ;; $block_26
                            end ;; $if_107
                            get_local $3
                            get_local $6
                            i32.eq
                            if $if_109
                              i32.const 1076
                              i32.const 1076
                              i32.load
                              i32.const 1
                              get_local $5
                              i32.shl
                              i32.const -1
                              i32.xor
                              i32.and
                              i32.store
                              br $block_25
                            end ;; $if_109
                            get_local $0
                            get_local $3
                            i32.eq
                            if $if_110
                              get_local $3
                              i32.const 8
                              i32.add
                              set_local $20
                            else
                              block $block_27
                                get_local $2
                                get_local $3
                                i32.gt_u
                                if $if_111
                                  call $_abort
                                end ;; $if_111
                                get_local $1
                                get_local $3
                                i32.const 8
                                i32.add
                                tee_local $0
                                i32.load
                                i32.eq
                                if $if_112
                                  get_local $0
                                  set_local $20
                                  br $block_27
                                end ;; $if_112
                                call $_abort
                              end ;; $block_27
                            end ;; $if_110
                            get_local $6
                            get_local $3
                            i32.store offset=12
                            get_local $20
                            get_local $6
                            i32.store
                          else
                            get_local $1
                            i32.load offset=24
                            set_local $10
                            get_local $1
                            get_local $1
                            i32.load offset=12
                            tee_local $0
                            i32.eq
                            if $if_113
                              block $block_28
                                get_local $1
                                i32.const 16
                                i32.add
                                tee_local $3
                                i32.const 4
                                i32.add
                                tee_local $6
                                i32.load
                                tee_local $0
                                if $if_114
                                  get_local $6
                                  set_local $3
                                else
                                  get_local $3
                                  i32.load
                                  tee_local $0
                                  i32.eqz
                                  br_if $block_28
                                end ;; $if_114
                                loop $loop_8
                                  block $block_29
                                    get_local $0
                                    i32.const 20
                                    i32.add
                                    tee_local $6
                                    i32.load
                                    tee_local $5
                                    i32.eqz
                                    if $if_115
                                      get_local $0
                                      i32.const 16
                                      i32.add
                                      tee_local $6
                                      i32.load
                                      tee_local $5
                                      i32.eqz
                                      br_if $block_29
                                    end ;; $if_115
                                    get_local $6
                                    set_local $3
                                    get_local $5
                                    set_local $0
                                    br $loop_8
                                  end ;; $block_29
                                end ;; $loop_8
                                get_local $2
                                get_local $3
                                i32.gt_u
                                if $if_116
                                  call $_abort
                                else
                                  get_local $3
                                  i32.const 0
                                  i32.store
                                  get_local $0
                                  set_local $9
                                end ;; $if_116
                              end ;; $block_28
                            else
                              get_local $2
                              get_local $1
                              i32.load offset=8
                              tee_local $3
                              i32.gt_u
                              if $if_117
                                call $_abort
                              end ;; $if_117
                              get_local $3
                              i32.const 12
                              i32.add
                              tee_local $2
                              i32.load
                              get_local $1
                              i32.ne
                              if $if_118
                                call $_abort
                              end ;; $if_118
                              get_local $1
                              get_local $0
                              i32.const 8
                              i32.add
                              tee_local $6
                              i32.load
                              i32.eq
                              if $if_119
                                get_local $2
                                get_local $0
                                i32.store
                                get_local $6
                                get_local $3
                                i32.store
                                get_local $0
                                set_local $9
                              else
                                call $_abort
                              end ;; $if_119
                            end ;; $if_113
                            get_local $10
                            i32.eqz
                            br_if $block_25
                            get_local $1
                            get_local $1
                            i32.load offset=28
                            tee_local $0
                            i32.const 2
                            i32.shl
                            i32.const 1380
                            i32.add
                            tee_local $3
                            i32.load
                            i32.eq
                            if $if_120
                              block $block_30
                                get_local $3
                                get_local $9
                                i32.store
                                get_local $9
                                br_if $block_30
                                i32.const 1080
                                i32.const 1080
                                i32.load
                                i32.const 1
                                get_local $0
                                i32.shl
                                i32.const -1
                                i32.xor
                                i32.and
                                i32.store
                                br $block_25
                              end ;; $block_30
                            else
                              i32.const 1092
                              i32.load
                              get_local $10
                              i32.gt_u
                              if $if_121
                                call $_abort
                              else
                                get_local $10
                                i32.const 16
                                i32.add
                                tee_local $0
                                get_local $10
                                i32.const 20
                                i32.add
                                get_local $1
                                get_local $0
                                i32.load
                                i32.eq
                                select
                                get_local $9
                                i32.store
                                get_local $9
                                i32.eqz
                                br_if $block_25
                              end ;; $if_121
                            end ;; $if_120
                            i32.const 1092
                            i32.load
                            tee_local $3
                            get_local $9
                            i32.gt_u
                            if $if_122
                              call $_abort
                            end ;; $if_122
                            get_local $9
                            get_local $10
                            i32.store offset=24
                            get_local $1
                            i32.const 16
                            i32.add
                            tee_local $2
                            i32.load
                            tee_local $0
                            if $if_123
                              get_local $3
                              get_local $0
                              i32.gt_u
                              if $if_124
                                call $_abort
                              else
                                get_local $9
                                get_local $0
                                i32.store offset=16
                                get_local $0
                                get_local $9
                                i32.store offset=24
                              end ;; $if_124
                            end ;; $if_123
                            get_local $2
                            i32.load offset=4
                            tee_local $0
                            i32.eqz
                            br_if $block_25
                            i32.const 1092
                            i32.load
                            get_local $0
                            i32.gt_u
                            if $if_125
                              call $_abort
                            else
                              get_local $9
                              get_local $0
                              i32.store offset=20
                              get_local $0
                              get_local $9
                              i32.store offset=24
                            end ;; $if_125
                          end ;; $if_106
                        end ;; $block_25
                        get_local $1
                        get_local $13
                        i32.add
                        set_local $1
                        get_local $4
                        get_local $13
                        i32.add
                      else
                        get_local $4
                      end ;; $if_105
                      set_local $2
                      get_local $1
                      i32.const 4
                      i32.add
                      tee_local $0
                      get_local $0
                      i32.load
                      i32.const -2
                      i32.and
                      i32.store
                      get_local $7
                      get_local $2
                      i32.const 1
                      i32.or
                      i32.store offset=4
                      get_local $2
                      get_local $7
                      i32.add
                      get_local $2
                      i32.store
                      get_local $2
                      i32.const 3
                      i32.shr_u
                      set_local $3
                      get_local $2
                      i32.const 256
                      i32.lt_u
                      if $if_126
                        get_local $3
                        i32.const 3
                        i32.shl
                        i32.const 1116
                        i32.add
                        set_local $0
                        i32.const 1076
                        i32.load
                        tee_local $1
                        i32.const 1
                        get_local $3
                        i32.shl
                        tee_local $3
                        i32.and
                        if $if_127
                          block $block_31
                            i32.const 1092
                            i32.load
                            get_local $0
                            i32.const 8
                            i32.add
                            tee_local $3
                            i32.load
                            tee_local $1
                            i32.le_u
                            if $if_128
                              get_local $1
                              set_local $15
                              get_local $3
                              set_local $21
                              br $block_31
                            end ;; $if_128
                            call $_abort
                          end ;; $block_31
                        else
                          i32.const 1076
                          get_local $1
                          get_local $3
                          i32.or
                          i32.store
                          get_local $0
                          set_local $15
                          get_local $0
                          i32.const 8
                          i32.add
                          set_local $21
                        end ;; $if_127
                        get_local $21
                        get_local $7
                        i32.store
                        get_local $15
                        get_local $7
                        i32.store offset=12
                        get_local $7
                        get_local $15
                        i32.store offset=8
                        get_local $7
                        get_local $0
                        i32.store offset=12
                        br $block_24
                      end ;; $if_126
                      get_local $2
                      i32.const 8
                      i32.shr_u
                      tee_local $0
                      if $if_129 (result i32)
                        get_local $2
                        i32.const 16777215
                        i32.gt_u
                        if $if_130 (result i32)
                          i32.const 31
                        else
                          get_local $0
                          get_local $0
                          i32.const 1048320
                          i32.add
                          i32.const 16
                          i32.shr_u
                          i32.const 8
                          i32.and
                          tee_local $3
                          i32.shl
                          tee_local $1
                          i32.const 520192
                          i32.add
                          i32.const 16
                          i32.shr_u
                          i32.const 4
                          i32.and
                          set_local $0
                          i32.const 14
                          get_local $0
                          get_local $3
                          i32.or
                          get_local $1
                          get_local $0
                          i32.shl
                          tee_local $0
                          i32.const 245760
                          i32.add
                          i32.const 16
                          i32.shr_u
                          i32.const 2
                          i32.and
                          tee_local $3
                          i32.or
                          i32.sub
                          get_local $0
                          get_local $3
                          i32.shl
                          i32.const 15
                          i32.shr_u
                          i32.add
                          tee_local $0
                          i32.const 1
                          i32.shl
                          get_local $2
                          get_local $0
                          i32.const 7
                          i32.add
                          i32.shr_u
                          i32.const 1
                          i32.and
                          i32.or
                        end ;; $if_130
                      else
                        i32.const 0
                      end ;; $if_129
                      tee_local $3
                      i32.const 2
                      i32.shl
                      i32.const 1380
                      i32.add
                      set_local $0
                      get_local $7
                      get_local $3
                      i32.store offset=28
                      get_local $7
                      i32.const 16
                      i32.add
                      tee_local $1
                      i32.const 0
                      i32.store offset=4
                      get_local $1
                      i32.const 0
                      i32.store
                      i32.const 1080
                      i32.load
                      tee_local $1
                      i32.const 1
                      get_local $3
                      i32.shl
                      tee_local $4
                      i32.and
                      i32.eqz
                      if $if_131
                        i32.const 1080
                        get_local $1
                        get_local $4
                        i32.or
                        i32.store
                        get_local $0
                        get_local $7
                        i32.store
                        get_local $7
                        get_local $0
                        i32.store offset=24
                        get_local $7
                        get_local $7
                        i32.store offset=12
                        get_local $7
                        get_local $7
                        i32.store offset=8
                        br $block_24
                      end ;; $if_131
                      get_local $2
                      get_local $0
                      i32.load
                      tee_local $0
                      i32.load offset=4
                      i32.const -8
                      i32.and
                      i32.eq
                      if $if_132
                        get_local $0
                        set_local $11
                      else
                        block $block_32
                          get_local $2
                          i32.const 0
                          i32.const 25
                          get_local $3
                          i32.const 1
                          i32.shr_u
                          i32.sub
                          get_local $3
                          i32.const 31
                          i32.eq
                          select
                          i32.shl
                          set_local $1
                          loop $loop_9
                            get_local $0
                            i32.const 16
                            i32.add
                            get_local $1
                            i32.const 31
                            i32.shr_u
                            i32.const 2
                            i32.shl
                            i32.add
                            tee_local $4
                            i32.load
                            tee_local $3
                            if $if_133
                              get_local $1
                              i32.const 1
                              i32.shl
                              set_local $1
                              get_local $2
                              get_local $3
                              i32.load offset=4
                              i32.const -8
                              i32.and
                              i32.eq
                              if $if_134
                                get_local $3
                                set_local $11
                                br $block_32
                              else
                                get_local $3
                                set_local $0
                                br $loop_9
                              end ;; $if_134
                              unreachable
                            end ;; $if_133
                          end ;; $loop_9
                          i32.const 1092
                          i32.load
                          get_local $4
                          i32.gt_u
                          if $if_135
                            call $_abort
                          else
                            get_local $4
                            get_local $7
                            i32.store
                            get_local $7
                            get_local $0
                            i32.store offset=24
                            get_local $7
                            get_local $7
                            i32.store offset=12
                            get_local $7
                            get_local $7
                            i32.store offset=8
                            br $block_24
                          end ;; $if_135
                        end ;; $block_32
                      end ;; $if_132
                      i32.const 1092
                      i32.load
                      tee_local $0
                      get_local $11
                      i32.le_u
                      get_local $0
                      get_local $11
                      i32.const 8
                      i32.add
                      tee_local $3
                      i32.load
                      tee_local $0
                      i32.le_u
                      i32.and
                      if $if_136
                        get_local $0
                        get_local $7
                        i32.store offset=12
                        get_local $3
                        get_local $7
                        i32.store
                        get_local $7
                        get_local $0
                        i32.store offset=8
                        get_local $7
                        get_local $11
                        i32.store offset=12
                        get_local $7
                        i32.const 0
                        i32.store offset=24
                      else
                        call $_abort
                      end ;; $if_136
                    end ;; $block_24
                  end ;; $if_103
                  get_local $14
                  set_global $17
                  get_local $8
                  i32.const 8
                  i32.add
                  return
                end ;; $if_102
              end ;; $block_22
              i32.const 1524
              set_local $2
              loop $loop_10
                block $block_33
                  get_local $2
                  i32.load
                  tee_local $4
                  get_local $6
                  i32.le_u
                  if $if_137
                    get_local $4
                    get_local $2
                    i32.load offset=4
                    i32.add
                    tee_local $5
                    get_local $6
                    i32.gt_u
                    br_if $block_33
                  end ;; $if_137
                  get_local $2
                  i32.load offset=8
                  set_local $2
                  br $loop_10
                end ;; $block_33
              end ;; $loop_10
              get_local $5
              i32.const -47
              i32.add
              tee_local $4
              i32.const 8
              i32.add
              set_local $2
              get_local $6
              get_local $4
              i32.const 0
              get_local $2
              i32.sub
              i32.const 7
              i32.and
              i32.const 0
              get_local $2
              i32.const 7
              i32.and
              select
              i32.add
              tee_local $2
              get_local $2
              get_local $6
              i32.const 16
              i32.add
              tee_local $9
              i32.lt_u
              select
              tee_local $2
              i32.const 8
              i32.add
              set_local $4
              i32.const 1100
              get_local $0
              i32.const 0
              get_local $0
              i32.const 8
              i32.add
              tee_local $7
              i32.sub
              i32.const 7
              i32.and
              i32.const 0
              get_local $7
              i32.const 7
              i32.and
              select
              tee_local $7
              i32.add
              tee_local $10
              i32.store
              i32.const 1088
              get_local $1
              i32.const -40
              i32.add
              tee_local $11
              get_local $7
              i32.sub
              tee_local $7
              i32.store
              get_local $10
              get_local $7
              i32.const 1
              i32.or
              i32.store offset=4
              get_local $0
              get_local $11
              i32.add
              i32.const 40
              i32.store offset=4
              i32.const 1104
              i32.const 1564
              i32.load
              i32.store
              get_local $2
              i32.const 4
              i32.add
              tee_local $7
              i32.const 27
              i32.store
              get_local $4
              i32.const 1524
              i64.load align=4
              i64.store align=4
              get_local $4
              i32.const 1532
              i64.load align=4
              i64.store offset=8 align=4
              i32.const 1524
              get_local $0
              i32.store
              i32.const 1528
              get_local $1
              i32.store
              i32.const 1536
              i32.const 0
              i32.store
              i32.const 1532
              get_local $4
              i32.store
              get_local $2
              i32.const 24
              i32.add
              set_local $0
              loop $loop_11
                get_local $0
                i32.const 4
                i32.add
                tee_local $1
                i32.const 7
                i32.store
                get_local $0
                i32.const 8
                i32.add
                get_local $5
                i32.lt_u
                if $if_138
                  get_local $1
                  set_local $0
                  br $loop_11
                end ;; $if_138
              end ;; $loop_11
              get_local $2
              get_local $6
              i32.ne
              if $if_139
                get_local $7
                get_local $7
                i32.load
                i32.const -2
                i32.and
                i32.store
                get_local $6
                get_local $2
                get_local $6
                i32.sub
                tee_local $4
                i32.const 1
                i32.or
                i32.store offset=4
                get_local $2
                get_local $4
                i32.store
                get_local $4
                i32.const 3
                i32.shr_u
                set_local $1
                get_local $4
                i32.const 256
                i32.lt_u
                if $if_140
                  get_local $1
                  i32.const 3
                  i32.shl
                  i32.const 1116
                  i32.add
                  set_local $0
                  i32.const 1076
                  i32.load
                  tee_local $2
                  i32.const 1
                  get_local $1
                  i32.shl
                  tee_local $1
                  i32.and
                  if $if_141
                    i32.const 1092
                    i32.load
                    get_local $0
                    i32.const 8
                    i32.add
                    tee_local $1
                    i32.load
                    tee_local $2
                    i32.gt_u
                    if $if_142
                      call $_abort
                    else
                      get_local $2
                      set_local $17
                      get_local $1
                      set_local $22
                    end ;; $if_142
                  else
                    i32.const 1076
                    get_local $1
                    get_local $2
                    i32.or
                    i32.store
                    get_local $0
                    set_local $17
                    get_local $0
                    i32.const 8
                    i32.add
                    set_local $22
                  end ;; $if_141
                  get_local $22
                  get_local $6
                  i32.store
                  get_local $17
                  get_local $6
                  i32.store offset=12
                  get_local $6
                  get_local $17
                  i32.store offset=8
                  get_local $6
                  get_local $0
                  i32.store offset=12
                  br $block_19
                end ;; $if_140
                get_local $4
                i32.const 8
                i32.shr_u
                tee_local $0
                if $if_143 (result i32)
                  get_local $4
                  i32.const 16777215
                  i32.gt_u
                  if $if_144 (result i32)
                    i32.const 31
                  else
                    get_local $0
                    get_local $0
                    i32.const 1048320
                    i32.add
                    i32.const 16
                    i32.shr_u
                    i32.const 8
                    i32.and
                    tee_local $1
                    i32.shl
                    tee_local $2
                    i32.const 520192
                    i32.add
                    i32.const 16
                    i32.shr_u
                    i32.const 4
                    i32.and
                    set_local $0
                    i32.const 14
                    get_local $2
                    get_local $0
                    i32.shl
                    tee_local $2
                    i32.const 245760
                    i32.add
                    i32.const 16
                    i32.shr_u
                    i32.const 2
                    i32.and
                    tee_local $5
                    get_local $0
                    get_local $1
                    i32.or
                    i32.or
                    i32.sub
                    get_local $2
                    get_local $5
                    i32.shl
                    i32.const 15
                    i32.shr_u
                    i32.add
                    tee_local $0
                    i32.const 1
                    i32.shl
                    get_local $4
                    get_local $0
                    i32.const 7
                    i32.add
                    i32.shr_u
                    i32.const 1
                    i32.and
                    i32.or
                  end ;; $if_144
                else
                  i32.const 0
                end ;; $if_143
                tee_local $1
                i32.const 2
                i32.shl
                i32.const 1380
                i32.add
                set_local $0
                get_local $6
                get_local $1
                i32.store offset=28
                get_local $6
                i32.const 0
                i32.store offset=20
                get_local $9
                i32.const 0
                i32.store
                i32.const 1080
                i32.load
                tee_local $2
                i32.const 1
                get_local $1
                i32.shl
                tee_local $5
                i32.and
                i32.eqz
                if $if_145
                  i32.const 1080
                  get_local $2
                  get_local $5
                  i32.or
                  i32.store
                  get_local $0
                  get_local $6
                  i32.store
                  get_local $6
                  get_local $0
                  i32.store offset=24
                  get_local $6
                  get_local $6
                  i32.store offset=12
                  get_local $6
                  get_local $6
                  i32.store offset=8
                  br $block_19
                end ;; $if_145
                get_local $0
                i32.load
                tee_local $0
                i32.load offset=4
                i32.const -8
                i32.and
                get_local $4
                i32.eq
                if $if_146
                  get_local $0
                  set_local $8
                else
                  block $block_34
                    get_local $4
                    i32.const 0
                    i32.const 25
                    get_local $1
                    i32.const 1
                    i32.shr_u
                    i32.sub
                    get_local $1
                    i32.const 31
                    i32.eq
                    select
                    i32.shl
                    set_local $2
                    loop $loop_12
                      get_local $0
                      i32.const 16
                      i32.add
                      get_local $2
                      i32.const 31
                      i32.shr_u
                      i32.const 2
                      i32.shl
                      i32.add
                      tee_local $5
                      i32.load
                      tee_local $1
                      if $if_147
                        get_local $2
                        i32.const 1
                        i32.shl
                        set_local $2
                        get_local $1
                        i32.load offset=4
                        i32.const -8
                        i32.and
                        get_local $4
                        i32.eq
                        if $if_148
                          get_local $1
                          set_local $8
                          br $block_34
                        else
                          get_local $1
                          set_local $0
                          br $loop_12
                        end ;; $if_148
                        unreachable
                      end ;; $if_147
                    end ;; $loop_12
                    i32.const 1092
                    i32.load
                    get_local $5
                    i32.gt_u
                    if $if_149
                      call $_abort
                    else
                      get_local $5
                      get_local $6
                      i32.store
                      get_local $6
                      get_local $0
                      i32.store offset=24
                      get_local $6
                      get_local $6
                      i32.store offset=12
                      get_local $6
                      get_local $6
                      i32.store offset=8
                      br $block_19
                    end ;; $if_149
                  end ;; $block_34
                end ;; $if_146
                i32.const 1092
                i32.load
                tee_local $0
                get_local $8
                i32.le_u
                get_local $0
                get_local $8
                i32.const 8
                i32.add
                tee_local $1
                i32.load
                tee_local $0
                i32.le_u
                i32.and
                if $if_150
                  get_local $0
                  get_local $6
                  i32.store offset=12
                  get_local $1
                  get_local $6
                  i32.store
                  get_local $6
                  get_local $0
                  i32.store offset=8
                  get_local $6
                  get_local $8
                  i32.store offset=12
                  get_local $6
                  i32.const 0
                  i32.store offset=24
                else
                  call $_abort
                end ;; $if_150
              end ;; $if_139
            end ;; $block_19
          else
            i32.const 1092
            i32.load
            tee_local $2
            i32.eqz
            get_local $0
            get_local $2
            i32.lt_u
            i32.or
            if $if_151
              i32.const 1092
              get_local $0
              i32.store
            end ;; $if_151
            i32.const 1524
            get_local $0
            i32.store
            i32.const 1528
            get_local $1
            i32.store
            i32.const 1536
            i32.const 0
            i32.store
            i32.const 1112
            i32.const 1548
            i32.load
            i32.store
            i32.const 1108
            i32.const -1
            i32.store
            i32.const 1128
            i32.const 1116
            i32.store
            i32.const 1124
            i32.const 1116
            i32.store
            i32.const 1136
            i32.const 1124
            i32.store
            i32.const 1132
            i32.const 1124
            i32.store
            i32.const 1144
            i32.const 1132
            i32.store
            i32.const 1140
            i32.const 1132
            i32.store
            i32.const 1152
            i32.const 1140
            i32.store
            i32.const 1148
            i32.const 1140
            i32.store
            i32.const 1160
            i32.const 1148
            i32.store
            i32.const 1156
            i32.const 1148
            i32.store
            i32.const 1168
            i32.const 1156
            i32.store
            i32.const 1164
            i32.const 1156
            i32.store
            i32.const 1176
            i32.const 1164
            i32.store
            i32.const 1172
            i32.const 1164
            i32.store
            i32.const 1184
            i32.const 1172
            i32.store
            i32.const 1180
            i32.const 1172
            i32.store
            i32.const 1192
            i32.const 1180
            i32.store
            i32.const 1188
            i32.const 1180
            i32.store
            i32.const 1200
            i32.const 1188
            i32.store
            i32.const 1196
            i32.const 1188
            i32.store
            i32.const 1208
            i32.const 1196
            i32.store
            i32.const 1204
            i32.const 1196
            i32.store
            i32.const 1216
            i32.const 1204
            i32.store
            i32.const 1212
            i32.const 1204
            i32.store
            i32.const 1224
            i32.const 1212
            i32.store
            i32.const 1220
            i32.const 1212
            i32.store
            i32.const 1232
            i32.const 1220
            i32.store
            i32.const 1228
            i32.const 1220
            i32.store
            i32.const 1240
            i32.const 1228
            i32.store
            i32.const 1236
            i32.const 1228
            i32.store
            i32.const 1248
            i32.const 1236
            i32.store
            i32.const 1244
            i32.const 1236
            i32.store
            i32.const 1256
            i32.const 1244
            i32.store
            i32.const 1252
            i32.const 1244
            i32.store
            i32.const 1264
            i32.const 1252
            i32.store
            i32.const 1260
            i32.const 1252
            i32.store
            i32.const 1272
            i32.const 1260
            i32.store
            i32.const 1268
            i32.const 1260
            i32.store
            i32.const 1280
            i32.const 1268
            i32.store
            i32.const 1276
            i32.const 1268
            i32.store
            i32.const 1288
            i32.const 1276
            i32.store
            i32.const 1284
            i32.const 1276
            i32.store
            i32.const 1296
            i32.const 1284
            i32.store
            i32.const 1292
            i32.const 1284
            i32.store
            i32.const 1304
            i32.const 1292
            i32.store
            i32.const 1300
            i32.const 1292
            i32.store
            i32.const 1312
            i32.const 1300
            i32.store
            i32.const 1308
            i32.const 1300
            i32.store
            i32.const 1320
            i32.const 1308
            i32.store
            i32.const 1316
            i32.const 1308
            i32.store
            i32.const 1328
            i32.const 1316
            i32.store
            i32.const 1324
            i32.const 1316
            i32.store
            i32.const 1336
            i32.const 1324
            i32.store
            i32.const 1332
            i32.const 1324
            i32.store
            i32.const 1344
            i32.const 1332
            i32.store
            i32.const 1340
            i32.const 1332
            i32.store
            i32.const 1352
            i32.const 1340
            i32.store
            i32.const 1348
            i32.const 1340
            i32.store
            i32.const 1360
            i32.const 1348
            i32.store
            i32.const 1356
            i32.const 1348
            i32.store
            i32.const 1368
            i32.const 1356
            i32.store
            i32.const 1364
            i32.const 1356
            i32.store
            i32.const 1376
            i32.const 1364
            i32.store
            i32.const 1372
            i32.const 1364
            i32.store
            i32.const 1100
            get_local $0
            i32.const 0
            get_local $0
            i32.const 8
            i32.add
            tee_local $2
            i32.sub
            i32.const 7
            i32.and
            i32.const 0
            get_local $2
            i32.const 7
            i32.and
            select
            tee_local $2
            i32.add
            tee_local $4
            i32.store
            i32.const 1088
            get_local $1
            i32.const -40
            i32.add
            tee_local $1
            get_local $2
            i32.sub
            tee_local $2
            i32.store
            get_local $4
            get_local $2
            i32.const 1
            i32.or
            i32.store offset=4
            get_local $0
            get_local $1
            i32.add
            i32.const 40
            i32.store offset=4
            i32.const 1104
            i32.const 1564
            i32.load
            i32.store
          end ;; $if_98
          i32.const 1088
          i32.load
          tee_local $0
          get_local $3
          i32.gt_u
          if $if_152
            i32.const 1088
            get_local $0
            get_local $3
            i32.sub
            tee_local $1
            i32.store
            br $block_12
          end ;; $if_152
        end ;; $block_13
        call $___errno_location
        i32.const 12
        i32.store
        get_local $14
        set_global $17
        i32.const 0
        return
      end ;; $block_12
      i32.const 1100
      get_local $3
      i32.const 1100
      i32.load
      tee_local $0
      i32.add
      tee_local $2
      i32.store
      get_local $2
      get_local $1
      i32.const 1
      i32.or
      i32.store offset=4
      get_local $0
      get_local $3
      i32.const 3
      i32.or
      i32.store offset=4
    end ;; $block_11
    get_local $14
    set_global $17
    get_local $0
    i32.const 8
    i32.add
    )
  
  (func $_free (type $1)
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
    get_local $0
    i32.eqz
    if $if
      return
    end ;; $if
    get_local $0
    i32.const -8
    i32.add
    tee_local $4
    i32.const 1092
    i32.load
    tee_local $12
    i32.lt_u
    if $if_0
      call $_abort
    end ;; $if_0
    get_local $0
    i32.const -4
    i32.add
    i32.load
    tee_local $0
    i32.const 3
    i32.and
    tee_local $11
    i32.const 1
    i32.eq
    if $if_1
      call $_abort
    end ;; $if_1
    get_local $4
    get_local $0
    i32.const -8
    i32.and
    tee_local $2
    i32.add
    set_local $7
    get_local $0
    i32.const 1
    i32.and
    if $if_2
      get_local $2
      set_local $1
      get_local $4
      tee_local $3
      set_local $5
    else
      block $block
        get_local $4
        i32.load
        set_local $9
        get_local $11
        i32.eqz
        if $if_3
          return
        end ;; $if_3
        get_local $4
        get_local $9
        i32.sub
        tee_local $0
        get_local $12
        i32.lt_u
        if $if_4
          call $_abort
        end ;; $if_4
        get_local $2
        get_local $9
        i32.add
        set_local $4
        get_local $0
        i32.const 1096
        i32.load
        i32.eq
        if $if_5
          get_local $7
          i32.const 4
          i32.add
          tee_local $3
          i32.load
          tee_local $1
          i32.const 3
          i32.and
          i32.const 3
          i32.ne
          if $if_6
            get_local $4
            set_local $1
            get_local $0
            tee_local $3
            set_local $5
            br $block
          end ;; $if_6
          i32.const 1084
          get_local $4
          i32.store
          get_local $3
          get_local $1
          i32.const -2
          i32.and
          i32.store
          get_local $0
          get_local $4
          i32.const 1
          i32.or
          i32.store offset=4
          get_local $0
          get_local $4
          i32.add
          get_local $4
          i32.store
          return
        end ;; $if_5
        get_local $9
        i32.const 3
        i32.shr_u
        set_local $2
        get_local $9
        i32.const 256
        i32.lt_u
        if $if_7
          get_local $0
          i32.load offset=12
          set_local $1
          get_local $0
          i32.load offset=8
          tee_local $5
          get_local $2
          i32.const 3
          i32.shl
          i32.const 1116
          i32.add
          tee_local $3
          i32.ne
          if $if_8
            get_local $12
            get_local $5
            i32.gt_u
            if $if_9
              call $_abort
            end ;; $if_9
            get_local $5
            i32.load offset=12
            get_local $0
            i32.ne
            if $if_10
              call $_abort
            end ;; $if_10
          end ;; $if_8
          get_local $1
          get_local $5
          i32.eq
          if $if_11
            i32.const 1076
            i32.const 1076
            i32.load
            i32.const 1
            get_local $2
            i32.shl
            i32.const -1
            i32.xor
            i32.and
            i32.store
            get_local $4
            set_local $1
            get_local $0
            tee_local $3
            set_local $5
            br $block
          end ;; $if_11
          get_local $1
          get_local $3
          i32.eq
          if $if_12
            get_local $1
            i32.const 8
            i32.add
            set_local $6
          else
            get_local $12
            get_local $1
            i32.gt_u
            if $if_13
              call $_abort
            end ;; $if_13
            get_local $0
            get_local $1
            i32.const 8
            i32.add
            tee_local $3
            i32.load
            i32.eq
            if $if_14
              get_local $3
              set_local $6
            else
              call $_abort
            end ;; $if_14
          end ;; $if_12
          get_local $5
          get_local $1
          i32.store offset=12
          get_local $6
          get_local $5
          i32.store
          get_local $4
          set_local $1
          get_local $0
          tee_local $3
          set_local $5
          br $block
        end ;; $if_7
        get_local $0
        i32.load offset=24
        set_local $13
        get_local $0
        get_local $0
        i32.load offset=12
        tee_local $2
        i32.eq
        if $if_15
          block $block_0
            get_local $0
            i32.const 16
            i32.add
            tee_local $6
            i32.const 4
            i32.add
            tee_local $9
            i32.load
            tee_local $2
            if $if_16
              get_local $9
              set_local $6
            else
              get_local $6
              i32.load
              tee_local $2
              i32.eqz
              br_if $block_0
            end ;; $if_16
            loop $loop
              block $block_1
                get_local $2
                i32.const 20
                i32.add
                tee_local $9
                i32.load
                tee_local $11
                i32.eqz
                if $if_17
                  get_local $2
                  i32.const 16
                  i32.add
                  tee_local $9
                  i32.load
                  tee_local $11
                  i32.eqz
                  br_if $block_1
                end ;; $if_17
                get_local $9
                set_local $6
                get_local $11
                set_local $2
                br $loop
              end ;; $block_1
            end ;; $loop
            get_local $12
            get_local $6
            i32.gt_u
            if $if_18
              call $_abort
            else
              get_local $6
              i32.const 0
              i32.store
              get_local $2
              set_local $8
            end ;; $if_18
          end ;; $block_0
        else
          get_local $12
          get_local $0
          i32.load offset=8
          tee_local $6
          i32.gt_u
          if $if_19
            call $_abort
          end ;; $if_19
          get_local $6
          i32.const 12
          i32.add
          tee_local $9
          i32.load
          get_local $0
          i32.ne
          if $if_20
            call $_abort
          end ;; $if_20
          get_local $0
          get_local $2
          i32.const 8
          i32.add
          tee_local $11
          i32.load
          i32.eq
          if $if_21
            get_local $9
            get_local $2
            i32.store
            get_local $11
            get_local $6
            i32.store
            get_local $2
            set_local $8
          else
            call $_abort
          end ;; $if_21
        end ;; $if_15
        get_local $13
        if $if_22
          get_local $0
          get_local $0
          i32.load offset=28
          tee_local $2
          i32.const 2
          i32.shl
          i32.const 1380
          i32.add
          tee_local $6
          i32.load
          i32.eq
          if $if_23
            get_local $6
            get_local $8
            i32.store
            get_local $8
            i32.eqz
            if $if_24
              i32.const 1080
              i32.const 1080
              i32.load
              i32.const 1
              get_local $2
              i32.shl
              i32.const -1
              i32.xor
              i32.and
              i32.store
              get_local $4
              set_local $1
              get_local $0
              tee_local $3
              set_local $5
              br $block
            end ;; $if_24
          else
            i32.const 1092
            i32.load
            get_local $13
            i32.gt_u
            if $if_25
              call $_abort
            else
              get_local $13
              i32.const 16
              i32.add
              tee_local $2
              get_local $13
              i32.const 20
              i32.add
              get_local $0
              get_local $2
              i32.load
              i32.eq
              select
              get_local $8
              i32.store
              get_local $8
              i32.eqz
              if $if_26
                get_local $4
                set_local $1
                get_local $0
                tee_local $3
                set_local $5
                br $block
              end ;; $if_26
            end ;; $if_25
          end ;; $if_23
          i32.const 1092
          i32.load
          tee_local $6
          get_local $8
          i32.gt_u
          if $if_27
            call $_abort
          end ;; $if_27
          get_local $8
          get_local $13
          i32.store offset=24
          get_local $0
          i32.const 16
          i32.add
          tee_local $9
          i32.load
          tee_local $2
          if $if_28
            get_local $6
            get_local $2
            i32.gt_u
            if $if_29
              call $_abort
            else
              get_local $8
              get_local $2
              i32.store offset=16
              get_local $2
              get_local $8
              i32.store offset=24
            end ;; $if_29
          end ;; $if_28
          get_local $9
          i32.load offset=4
          tee_local $2
          if $if_30
            i32.const 1092
            i32.load
            get_local $2
            i32.gt_u
            if $if_31
              call $_abort
            else
              get_local $8
              get_local $2
              i32.store offset=20
              get_local $2
              get_local $8
              i32.store offset=24
              get_local $4
              set_local $1
              get_local $0
              tee_local $3
              set_local $5
            end ;; $if_31
          else
            get_local $4
            set_local $1
            get_local $0
            tee_local $3
            set_local $5
          end ;; $if_30
        else
          get_local $4
          set_local $1
          get_local $0
          tee_local $3
          set_local $5
        end ;; $if_22
      end ;; $block
    end ;; $if_2
    get_local $5
    get_local $7
    i32.ge_u
    if $if_32
      call $_abort
    end ;; $if_32
    get_local $7
    i32.const 4
    i32.add
    tee_local $4
    i32.load
    tee_local $0
    i32.const 1
    i32.and
    i32.eqz
    if $if_33
      call $_abort
    end ;; $if_33
    get_local $0
    i32.const 2
    i32.and
    if $if_34 (result i32)
      get_local $4
      get_local $0
      i32.const -2
      i32.and
      i32.store
      get_local $3
      get_local $1
      i32.const 1
      i32.or
      i32.store offset=4
      get_local $1
      get_local $5
      i32.add
      get_local $1
      i32.store
      get_local $1
    else
      get_local $7
      i32.const 1100
      i32.load
      i32.eq
      if $if_35
        i32.const 1088
        get_local $1
        i32.const 1088
        i32.load
        i32.add
        tee_local $0
        i32.store
        i32.const 1100
        get_local $3
        i32.store
        get_local $3
        get_local $0
        i32.const 1
        i32.or
        i32.store offset=4
        i32.const 1096
        i32.load
        get_local $3
        i32.ne
        if $if_36
          return
        end ;; $if_36
        i32.const 1096
        i32.const 0
        i32.store
        i32.const 1084
        i32.const 0
        i32.store
        return
      end ;; $if_35
      get_local $7
      i32.const 1096
      i32.load
      i32.eq
      if $if_37
        i32.const 1084
        get_local $1
        i32.const 1084
        i32.load
        i32.add
        tee_local $0
        i32.store
        i32.const 1096
        get_local $5
        i32.store
        get_local $3
        get_local $0
        i32.const 1
        i32.or
        i32.store offset=4
        get_local $0
        get_local $5
        i32.add
        get_local $0
        i32.store
        return
      end ;; $if_37
      get_local $1
      get_local $0
      i32.const -8
      i32.and
      i32.add
      set_local $4
      get_local $0
      i32.const 3
      i32.shr_u
      set_local $6
      block $block_2
        get_local $0
        i32.const 256
        i32.lt_u
        if $if_38
          get_local $7
          i32.load offset=12
          set_local $1
          get_local $7
          i32.load offset=8
          tee_local $2
          get_local $6
          i32.const 3
          i32.shl
          i32.const 1116
          i32.add
          tee_local $0
          i32.ne
          if $if_39
            i32.const 1092
            i32.load
            get_local $2
            i32.gt_u
            if $if_40
              call $_abort
            end ;; $if_40
            get_local $2
            i32.load offset=12
            get_local $7
            i32.ne
            if $if_41
              call $_abort
            end ;; $if_41
          end ;; $if_39
          get_local $1
          get_local $2
          i32.eq
          if $if_42
            i32.const 1076
            i32.const 1076
            i32.load
            i32.const 1
            get_local $6
            i32.shl
            i32.const -1
            i32.xor
            i32.and
            i32.store
            br $block_2
          end ;; $if_42
          get_local $0
          get_local $1
          i32.eq
          if $if_43
            get_local $1
            i32.const 8
            i32.add
            set_local $16
          else
            i32.const 1092
            i32.load
            get_local $1
            i32.gt_u
            if $if_44
              call $_abort
            end ;; $if_44
            get_local $7
            get_local $1
            i32.const 8
            i32.add
            tee_local $0
            i32.load
            i32.eq
            if $if_45
              get_local $0
              set_local $16
            else
              call $_abort
            end ;; $if_45
          end ;; $if_43
          get_local $2
          get_local $1
          i32.store offset=12
          get_local $16
          get_local $2
          i32.store
        else
          get_local $7
          i32.load offset=24
          set_local $8
          get_local $7
          get_local $7
          i32.load offset=12
          tee_local $0
          i32.eq
          if $if_46
            block $block_3
              get_local $7
              i32.const 16
              i32.add
              tee_local $1
              i32.const 4
              i32.add
              tee_local $2
              i32.load
              tee_local $0
              if $if_47
                get_local $2
                set_local $1
              else
                get_local $1
                i32.load
                tee_local $0
                i32.eqz
                br_if $block_3
              end ;; $if_47
              loop $loop_0
                block $block_4
                  get_local $0
                  i32.const 20
                  i32.add
                  tee_local $2
                  i32.load
                  tee_local $6
                  i32.eqz
                  if $if_48
                    get_local $0
                    i32.const 16
                    i32.add
                    tee_local $2
                    i32.load
                    tee_local $6
                    i32.eqz
                    br_if $block_4
                  end ;; $if_48
                  get_local $2
                  set_local $1
                  get_local $6
                  set_local $0
                  br $loop_0
                end ;; $block_4
              end ;; $loop_0
              i32.const 1092
              i32.load
              get_local $1
              i32.gt_u
              if $if_49
                call $_abort
              else
                get_local $1
                i32.const 0
                i32.store
                get_local $0
                set_local $10
              end ;; $if_49
            end ;; $block_3
          else
            i32.const 1092
            i32.load
            get_local $7
            i32.load offset=8
            tee_local $1
            i32.gt_u
            if $if_50
              call $_abort
            end ;; $if_50
            get_local $1
            i32.const 12
            i32.add
            tee_local $2
            i32.load
            get_local $7
            i32.ne
            if $if_51
              call $_abort
            end ;; $if_51
            get_local $7
            get_local $0
            i32.const 8
            i32.add
            tee_local $6
            i32.load
            i32.eq
            if $if_52
              get_local $2
              get_local $0
              i32.store
              get_local $6
              get_local $1
              i32.store
              get_local $0
              set_local $10
            else
              call $_abort
            end ;; $if_52
          end ;; $if_46
          get_local $8
          if $if_53
            get_local $7
            get_local $7
            i32.load offset=28
            tee_local $0
            i32.const 2
            i32.shl
            i32.const 1380
            i32.add
            tee_local $1
            i32.load
            i32.eq
            if $if_54
              get_local $1
              get_local $10
              i32.store
              get_local $10
              i32.eqz
              if $if_55
                i32.const 1080
                i32.const 1080
                i32.load
                i32.const 1
                get_local $0
                i32.shl
                i32.const -1
                i32.xor
                i32.and
                i32.store
                br $block_2
              end ;; $if_55
            else
              i32.const 1092
              i32.load
              get_local $8
              i32.gt_u
              if $if_56
                call $_abort
              else
                get_local $8
                i32.const 16
                i32.add
                tee_local $0
                get_local $8
                i32.const 20
                i32.add
                get_local $7
                get_local $0
                i32.load
                i32.eq
                select
                get_local $10
                i32.store
                get_local $10
                i32.eqz
                br_if $block_2
              end ;; $if_56
            end ;; $if_54
            i32.const 1092
            i32.load
            tee_local $1
            get_local $10
            i32.gt_u
            if $if_57
              call $_abort
            end ;; $if_57
            get_local $10
            get_local $8
            i32.store offset=24
            get_local $7
            i32.const 16
            i32.add
            tee_local $2
            i32.load
            tee_local $0
            if $if_58
              get_local $1
              get_local $0
              i32.gt_u
              if $if_59
                call $_abort
              else
                get_local $10
                get_local $0
                i32.store offset=16
                get_local $0
                get_local $10
                i32.store offset=24
              end ;; $if_59
            end ;; $if_58
            get_local $2
            i32.load offset=4
            tee_local $0
            if $if_60
              i32.const 1092
              i32.load
              get_local $0
              i32.gt_u
              if $if_61
                call $_abort
              else
                get_local $10
                get_local $0
                i32.store offset=20
                get_local $0
                get_local $10
                i32.store offset=24
              end ;; $if_61
            end ;; $if_60
          end ;; $if_53
        end ;; $if_38
      end ;; $block_2
      get_local $3
      get_local $4
      i32.const 1
      i32.or
      i32.store offset=4
      get_local $4
      get_local $5
      i32.add
      get_local $4
      i32.store
      get_local $3
      i32.const 1096
      i32.load
      i32.eq
      if $if_62 (result i32)
        i32.const 1084
        get_local $4
        i32.store
        return
      else
        get_local $4
      end ;; $if_62
    end ;; $if_34
    tee_local $5
    i32.const 3
    i32.shr_u
    set_local $1
    get_local $5
    i32.const 256
    i32.lt_u
    if $if_63
      get_local $1
      i32.const 3
      i32.shl
      i32.const 1116
      i32.add
      set_local $0
      i32.const 1076
      i32.load
      tee_local $5
      i32.const 1
      get_local $1
      i32.shl
      tee_local $1
      i32.and
      if $if_64
        i32.const 1092
        i32.load
        get_local $0
        i32.const 8
        i32.add
        tee_local $1
        i32.load
        tee_local $5
        i32.gt_u
        if $if_65
          call $_abort
        else
          get_local $5
          set_local $15
          get_local $1
          set_local $17
        end ;; $if_65
      else
        i32.const 1076
        get_local $1
        get_local $5
        i32.or
        i32.store
        get_local $0
        set_local $15
        get_local $0
        i32.const 8
        i32.add
        set_local $17
      end ;; $if_64
      get_local $17
      get_local $3
      i32.store
      get_local $15
      get_local $3
      i32.store offset=12
      get_local $3
      get_local $15
      i32.store offset=8
      get_local $3
      get_local $0
      i32.store offset=12
      return
    end ;; $if_63
    get_local $5
    i32.const 8
    i32.shr_u
    tee_local $0
    if $if_66 (result i32)
      get_local $5
      i32.const 16777215
      i32.gt_u
      if $if_67 (result i32)
        i32.const 31
      else
        get_local $0
        get_local $0
        i32.const 1048320
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 8
        i32.and
        tee_local $1
        i32.shl
        tee_local $4
        i32.const 520192
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 4
        i32.and
        set_local $0
        i32.const 14
        get_local $0
        get_local $1
        i32.or
        get_local $4
        get_local $0
        i32.shl
        tee_local $0
        i32.const 245760
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 2
        i32.and
        tee_local $1
        i32.or
        i32.sub
        get_local $0
        get_local $1
        i32.shl
        i32.const 15
        i32.shr_u
        i32.add
        tee_local $0
        i32.const 1
        i32.shl
        get_local $5
        get_local $0
        i32.const 7
        i32.add
        i32.shr_u
        i32.const 1
        i32.and
        i32.or
      end ;; $if_67
    else
      i32.const 0
    end ;; $if_66
    tee_local $1
    i32.const 2
    i32.shl
    i32.const 1380
    i32.add
    set_local $0
    get_local $3
    get_local $1
    i32.store offset=28
    get_local $3
    i32.const 0
    i32.store offset=20
    get_local $3
    i32.const 0
    i32.store offset=16
    i32.const 1080
    i32.load
    tee_local $4
    i32.const 1
    get_local $1
    i32.shl
    tee_local $2
    i32.and
    if $if_68
      block $block_5
        get_local $5
        get_local $0
        i32.load
        tee_local $0
        i32.load offset=4
        i32.const -8
        i32.and
        i32.eq
        if $if_69
          get_local $0
          set_local $14
        else
          block $block_6
            get_local $5
            i32.const 0
            i32.const 25
            get_local $1
            i32.const 1
            i32.shr_u
            i32.sub
            get_local $1
            i32.const 31
            i32.eq
            select
            i32.shl
            set_local $4
            loop $loop_1
              get_local $0
              i32.const 16
              i32.add
              get_local $4
              i32.const 31
              i32.shr_u
              i32.const 2
              i32.shl
              i32.add
              tee_local $2
              i32.load
              tee_local $1
              if $if_70
                get_local $4
                i32.const 1
                i32.shl
                set_local $4
                get_local $5
                get_local $1
                i32.load offset=4
                i32.const -8
                i32.and
                i32.eq
                if $if_71
                  get_local $1
                  set_local $14
                  br $block_6
                else
                  get_local $1
                  set_local $0
                  br $loop_1
                end ;; $if_71
                unreachable
              end ;; $if_70
            end ;; $loop_1
            i32.const 1092
            i32.load
            get_local $2
            i32.gt_u
            if $if_72
              call $_abort
            else
              get_local $2
              get_local $3
              i32.store
              get_local $3
              get_local $0
              i32.store offset=24
              get_local $3
              get_local $3
              i32.store offset=12
              get_local $3
              get_local $3
              i32.store offset=8
              br $block_5
            end ;; $if_72
          end ;; $block_6
        end ;; $if_69
        i32.const 1092
        i32.load
        tee_local $0
        get_local $14
        i32.le_u
        get_local $0
        get_local $14
        i32.const 8
        i32.add
        tee_local $1
        i32.load
        tee_local $0
        i32.le_u
        i32.and
        if $if_73
          get_local $0
          get_local $3
          i32.store offset=12
          get_local $1
          get_local $3
          i32.store
          get_local $3
          get_local $0
          i32.store offset=8
          get_local $3
          get_local $14
          i32.store offset=12
          get_local $3
          i32.const 0
          i32.store offset=24
        else
          call $_abort
        end ;; $if_73
      end ;; $block_5
    else
      i32.const 1080
      get_local $2
      get_local $4
      i32.or
      i32.store
      get_local $0
      get_local $3
      i32.store
      get_local $3
      get_local $0
      i32.store offset=24
      get_local $3
      get_local $3
      i32.store offset=12
      get_local $3
      get_local $3
      i32.store offset=8
    end ;; $if_68
    i32.const 1108
    i32.const 1108
    i32.load
    i32.const -1
    i32.add
    tee_local $0
    i32.store
    get_local $0
    if $if_74
      return
    end ;; $if_74
    i32.const 1532
    set_local $0
    loop $loop_2
      get_local $0
      i32.load
      tee_local $3
      i32.const 8
      i32.add
      set_local $0
      get_local $3
      br_if $loop_2
    end ;; $loop_2
    i32.const 1108
    i32.const -1
    i32.store
    )
  
  (func $__Znwm (type $5)
    (param $0 i32)
    (result i32)
    (local $1 i32)
    get_local $0
    i32.const 1
    get_local $0
    select
    set_local $1
    loop $loop
      get_local $1
      call $_malloc
      tee_local $0
      i32.eqz
      if $if
        call $__ZSt15get_new_handlerv
        if $if_0
          i32.const 0
          call_indirect $10 (type $0)
          br $loop
        else
          i32.const 0
          set_local $0
        end ;; $if_0
      end ;; $if
    end ;; $loop
    get_local $0
    )
  
  (func $__ZdlPv (type $1)
    (param $0 i32)
    get_local $0
    call $_free
    )
  
  (func $__ZNKSt3__221__basic_string_commonILb1EE20__throw_length_errorEv (type $0)
    call $_abort
    )
  
  (func $__ZNSt3__211char_traitsIcE4copyEPcPKcm (type $3)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    get_local $2
    if $if
      get_local $0
      get_local $1
      get_local $2
      call $_memcpy
      drop
    end ;; $if
    get_local $0
    )
  
  (func $__ZNSt3__211char_traitsIcE6assignERcRKc (type $6)
    (param $0 i32)
    (param $1 i32)
    get_local $0
    get_local $1
    i32.load8_s
    i32.store8
    )
  
  (func $__ZNSt3__211char_traitsIcE6lengthEPKc (type $5)
    (param $0 i32)
    (result i32)
    get_local $0
    call $_strlen
    )
  
  (func $__ZNSt3__211char_traitsIcE4moveEPcPKcm (type $3)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    get_local $2
    if $if
      get_local $0
      get_local $1
      get_local $2
      call $_memmove
      drop
    end ;; $if
    get_local $0
    )
  
  (func $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE21__grow_by_and_replaceEmmmmmmPKc (type $8)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (param $4 i32)
    (param $5 i32)
    (param $6 i32)
    (local $7 i32)
    (local $8 i32)
    (local $9 i32)
    get_global $17
    set_local $7
    get_global $17
    i32.const 16
    i32.add
    set_global $17
    i32.const -18
    get_local $1
    i32.sub
    get_local $2
    i32.lt_u
    if $if
      call $__ZNKSt3__221__basic_string_commonILb1EE20__throw_length_errorEv
    end ;; $if
    get_local $0
    i32.load8_s offset=11
    i32.const 0
    i32.lt_s
    if $if_0 (result i32)
      get_local $0
      i32.load
    else
      get_local $0
    end ;; $if_0
    set_local $8
    get_local $1
    i32.const 2147483623
    i32.lt_u
    if $if_1 (result i32)
      i32.const 11
      get_local $1
      i32.const 1
      i32.shl
      tee_local $9
      get_local $1
      get_local $2
      i32.add
      tee_local $2
      get_local $2
      get_local $9
      i32.lt_u
      select
      tee_local $2
      i32.const 16
      i32.add
      i32.const -16
      i32.and
      get_local $2
      i32.const 11
      i32.lt_u
      select
    else
      i32.const -17
    end ;; $if_1
    tee_local $9
    call $__Znwm
    set_local $2
    get_local $4
    if $if_2
      get_local $2
      get_local $8
      get_local $4
      call $__ZNSt3__211char_traitsIcE4copyEPcPKcm
      drop
    end ;; $if_2
    get_local $5
    if $if_3
      get_local $2
      get_local $4
      i32.add
      get_local $6
      get_local $5
      call $__ZNSt3__211char_traitsIcE4copyEPcPKcm
      drop
    end ;; $if_3
    get_local $3
    get_local $4
    i32.sub
    tee_local $6
    if $if_4
      get_local $5
      get_local $2
      get_local $4
      i32.add
      i32.add
      get_local $4
      get_local $8
      i32.add
      get_local $6
      call $__ZNSt3__211char_traitsIcE4copyEPcPKcm
      drop
    end ;; $if_4
    get_local $1
    i32.const 10
    i32.ne
    if $if_5
      get_local $8
      call $__ZdlPv
    end ;; $if_5
    get_local $0
    get_local $2
    i32.store
    get_local $0
    get_local $9
    i32.const -2147483648
    i32.or
    i32.store offset=8
    get_local $0
    get_local $3
    get_local $5
    i32.add
    tee_local $0
    i32.store offset=4
    get_local $7
    i32.const 0
    i32.store8
    get_local $0
    get_local $2
    i32.add
    get_local $7
    call $__ZNSt3__211char_traitsIcE6assignERcRKc
    get_local $7
    set_global $17
    )
  
  (func $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6appendEPKcm (type $3)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    (local $7 i32)
    get_global $17
    set_local $5
    get_global $17
    i32.const 16
    i32.add
    set_global $17
    get_local $0
    i32.const 11
    i32.add
    tee_local $7
    i32.load8_s
    tee_local $4
    i32.const 0
    i32.lt_s
    tee_local $6
    if $if (result i32)
      get_local $0
      i32.load offset=4
      set_local $3
      get_local $0
      i32.load offset=8
      i32.const 2147483647
      i32.and
      i32.const -1
      i32.add
    else
      get_local $4
      i32.const 255
      i32.and
      set_local $3
      i32.const 10
    end ;; $if
    tee_local $4
    get_local $3
    i32.sub
    get_local $2
    i32.lt_u
    if $if_0
      get_local $0
      get_local $4
      get_local $2
      get_local $3
      i32.add
      get_local $4
      i32.sub
      get_local $3
      get_local $3
      get_local $2
      get_local $1
      call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE21__grow_by_and_replaceEmmmmmmPKc
    else
      get_local $2
      if $if_1
        get_local $3
        get_local $6
        if $if_2 (result i32)
          get_local $0
          i32.load
        else
          get_local $0
        end ;; $if_2
        tee_local $6
        i32.add
        get_local $1
        get_local $2
        call $__ZNSt3__211char_traitsIcE4copyEPcPKcm
        drop
        get_local $2
        get_local $3
        i32.add
        set_local $1
        get_local $7
        i32.load8_s
        i32.const 0
        i32.lt_s
        if $if_3
          get_local $0
          get_local $1
          i32.store offset=4
        else
          get_local $7
          get_local $1
          i32.store8
        end ;; $if_3
        get_local $5
        i32.const 0
        i32.store8
        get_local $1
        get_local $6
        i32.add
        get_local $5
        call $__ZNSt3__211char_traitsIcE6assignERcRKc
      end ;; $if_1
    end ;; $if_0
    get_local $5
    set_global $17
    get_local $0
    )
  
  (func $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6appendEPKc (type $5)
    (param $0 i32)
    (result i32)
    get_local $0
    i32.const 1040
    i32.const 1040
    call $__ZNSt3__211char_traitsIcE6lengthEPKc
    call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6appendEPKcm
    )
  
  (func $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6insertEmPKcm (type $9)
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
    get_global $17
    set_local $7
    get_global $17
    i32.const 16
    i32.add
    set_global $17
    get_local $0
    i32.const 11
    i32.add
    tee_local $8
    i32.load8_s
    tee_local $4
    i32.const 0
    i32.lt_s
    tee_local $5
    if $if (result i32)
      get_local $0
      i32.load offset=4
    else
      get_local $4
      i32.const 255
      i32.and
    end ;; $if
    tee_local $6
    get_local $1
    i32.lt_u
    if $if_0
      call $__ZNKSt3__221__basic_string_commonILb1EE20__throw_length_errorEv
    end ;; $if_0
    get_local $5
    if $if_1 (result i32)
      get_local $0
      i32.load offset=8
      i32.const 2147483647
      i32.and
      i32.const -1
      i32.add
    else
      i32.const 10
    end ;; $if_1
    tee_local $4
    get_local $6
    i32.sub
    get_local $3
    i32.lt_u
    if $if_2
      get_local $0
      get_local $4
      get_local $3
      get_local $6
      i32.add
      get_local $4
      i32.sub
      get_local $6
      get_local $1
      get_local $3
      get_local $2
      call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE21__grow_by_and_replaceEmmmmmmPKc
    else
      get_local $3
      if $if_3
        get_local $1
        get_local $5
        if $if_4 (result i32)
          get_local $0
          i32.load
        else
          get_local $0
        end ;; $if_4
        tee_local $4
        i32.add
        set_local $5
        get_local $6
        get_local $1
        i32.sub
        tee_local $1
        if $if_5
          get_local $2
          get_local $3
          i32.add
          get_local $2
          get_local $5
          get_local $2
          i32.le_u
          get_local $4
          get_local $6
          i32.add
          get_local $2
          i32.gt_u
          i32.and
          select
          set_local $2
          get_local $3
          get_local $5
          i32.add
          get_local $5
          get_local $1
          call $__ZNSt3__211char_traitsIcE4moveEPcPKcm
          drop
        end ;; $if_5
        get_local $5
        get_local $2
        get_local $3
        call $__ZNSt3__211char_traitsIcE4moveEPcPKcm
        drop
        get_local $3
        get_local $6
        i32.add
        set_local $1
        get_local $8
        i32.load8_s
        i32.const 0
        i32.lt_s
        if $if_6
          get_local $0
          get_local $1
          i32.store offset=4
        else
          get_local $8
          get_local $1
          i32.store8
        end ;; $if_6
        get_local $7
        i32.const 0
        i32.store8
        get_local $1
        get_local $4
        i32.add
        get_local $7
        call $__ZNSt3__211char_traitsIcE6assignERcRKc
      end ;; $if_3
    end ;; $if_2
    get_local $7
    set_global $17
    get_local $0
    )
  
  (func $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6insertEmPKc (type $5)
    (param $0 i32)
    (result i32)
    get_local $0
    i32.const 0
    i32.const 1024
    i32.const 1024
    call $__ZNSt3__211char_traitsIcE6lengthEPKc
    call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6insertEmPKcm
    )
  
  (func $__ZSt15get_new_handlerv (type $2)
    (result i32)
    (local $0 i32)
    i32.const 1572
    i32.const 1572
    i32.load
    tee_local $0
    i32.store
    get_local $0
    )
  
  (func $_memcpy (type $3)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    get_local $2
    i32.const 8192
    i32.ge_s
    if $if
      get_local $0
      get_local $1
      get_local $2
      call $_emscripten_memcpy_big
      return
    end ;; $if
    get_local $0
    set_local $4
    get_local $0
    get_local $2
    i32.add
    set_local $3
    get_local $0
    i32.const 3
    i32.and
    get_local $1
    i32.const 3
    i32.and
    i32.eq
    if $if_0
      loop $loop
        get_local $0
        i32.const 3
        i32.and
        if $if_1
          get_local $2
          i32.eqz
          if $if_2
            get_local $4
            return
          end ;; $if_2
          get_local $0
          get_local $1
          i32.load8_s
          i32.store8
          get_local $0
          i32.const 1
          i32.add
          set_local $0
          get_local $1
          i32.const 1
          i32.add
          set_local $1
          get_local $2
          i32.const 1
          i32.sub
          set_local $2
          br $loop
        end ;; $if_1
      end ;; $loop
      get_local $3
      i32.const -4
      i32.and
      tee_local $2
      i32.const -64
      i32.add
      set_local $5
      loop $loop_0
        get_local $0
        get_local $5
        i32.le_s
        if $if_3
          get_local $0
          get_local $1
          i32.load
          i32.store
          get_local $0
          get_local $1
          i32.load offset=4
          i32.store offset=4
          get_local $0
          get_local $1
          i32.load offset=8
          i32.store offset=8
          get_local $0
          get_local $1
          i32.load offset=12
          i32.store offset=12
          get_local $0
          get_local $1
          i32.load offset=16
          i32.store offset=16
          get_local $0
          get_local $1
          i32.load offset=20
          i32.store offset=20
          get_local $0
          get_local $1
          i32.load offset=24
          i32.store offset=24
          get_local $0
          get_local $1
          i32.load offset=28
          i32.store offset=28
          get_local $0
          get_local $1
          i32.load offset=32
          i32.store offset=32
          get_local $0
          get_local $1
          i32.load offset=36
          i32.store offset=36
          get_local $0
          get_local $1
          i32.load offset=40
          i32.store offset=40
          get_local $0
          get_local $1
          i32.load offset=44
          i32.store offset=44
          get_local $0
          get_local $1
          i32.load offset=48
          i32.store offset=48
          get_local $0
          get_local $1
          i32.load offset=52
          i32.store offset=52
          get_local $0
          get_local $1
          i32.load offset=56
          i32.store offset=56
          get_local $0
          get_local $1
          i32.load offset=60
          i32.store offset=60
          get_local $0
          i32.const -64
          i32.sub
          set_local $0
          get_local $1
          i32.const -64
          i32.sub
          set_local $1
          br $loop_0
        end ;; $if_3
      end ;; $loop_0
      loop $loop_1
        get_local $0
        get_local $2
        i32.lt_s
        if $if_4
          get_local $0
          get_local $1
          i32.load
          i32.store
          get_local $0
          i32.const 4
          i32.add
          set_local $0
          get_local $1
          i32.const 4
          i32.add
          set_local $1
          br $loop_1
        end ;; $if_4
      end ;; $loop_1
    else
      get_local $3
      i32.const 4
      i32.sub
      set_local $2
      loop $loop_2
        get_local $0
        get_local $2
        i32.lt_s
        if $if_5
          get_local $0
          get_local $1
          i32.load8_s
          i32.store8
          get_local $0
          get_local $1
          i32.load8_s offset=1
          i32.store8 offset=1
          get_local $0
          get_local $1
          i32.load8_s offset=2
          i32.store8 offset=2
          get_local $0
          get_local $1
          i32.load8_s offset=3
          i32.store8 offset=3
          get_local $0
          i32.const 4
          i32.add
          set_local $0
          get_local $1
          i32.const 4
          i32.add
          set_local $1
          br $loop_2
        end ;; $if_5
      end ;; $loop_2
    end ;; $if_0
    loop $loop_3
      get_local $0
      get_local $3
      i32.lt_s
      if $if_6
        get_local $0
        get_local $1
        i32.load8_s
        i32.store8
        get_local $0
        i32.const 1
        i32.add
        set_local $0
        get_local $1
        i32.const 1
        i32.add
        set_local $1
        br $loop_3
      end ;; $if_6
    end ;; $loop_3
    get_local $4
    )
  
  (func $_memmove (type $3)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    (local $3 i32)
    get_local $1
    get_local $0
    i32.lt_s
    get_local $0
    get_local $1
    get_local $2
    i32.add
    i32.lt_s
    i32.and
    if $if
      get_local $1
      get_local $2
      i32.add
      set_local $1
      get_local $0
      tee_local $3
      get_local $2
      i32.add
      set_local $0
      loop $loop
        get_local $2
        i32.const 0
        i32.gt_s
        if $if_0
          get_local $2
          i32.const 1
          i32.sub
          set_local $2
          get_local $0
          i32.const 1
          i32.sub
          tee_local $0
          get_local $1
          i32.const 1
          i32.sub
          tee_local $1
          i32.load8_s
          i32.store8
          br $loop
        end ;; $if_0
      end ;; $loop
      get_local $3
      set_local $0
    else
      get_local $0
      get_local $1
      get_local $2
      call $_memcpy
      drop
    end ;; $if
    get_local $0
    )
  
  (func $_memset (type $3)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    get_local $0
    get_local $2
    i32.add
    set_local $4
    get_local $1
    i32.const 255
    i32.and
    set_local $1
    get_local $2
    i32.const 67
    i32.ge_s
    if $if
      loop $loop
        get_local $0
        i32.const 3
        i32.and
        if $if_0
          get_local $0
          get_local $1
          i32.store8
          get_local $0
          i32.const 1
          i32.add
          set_local $0
          br $loop
        end ;; $if_0
      end ;; $loop
      get_local $4
      i32.const -4
      i32.and
      tee_local $5
      i32.const -64
      i32.add
      set_local $6
      get_local $1
      i32.const 8
      i32.shl
      get_local $1
      i32.or
      get_local $1
      i32.const 16
      i32.shl
      i32.or
      get_local $1
      i32.const 24
      i32.shl
      i32.or
      set_local $3
      loop $loop_0
        get_local $0
        get_local $6
        i32.le_s
        if $if_1
          get_local $0
          get_local $3
          i32.store
          get_local $0
          get_local $3
          i32.store offset=4
          get_local $0
          get_local $3
          i32.store offset=8
          get_local $0
          get_local $3
          i32.store offset=12
          get_local $0
          get_local $3
          i32.store offset=16
          get_local $0
          get_local $3
          i32.store offset=20
          get_local $0
          get_local $3
          i32.store offset=24
          get_local $0
          get_local $3
          i32.store offset=28
          get_local $0
          get_local $3
          i32.store offset=32
          get_local $0
          get_local $3
          i32.store offset=36
          get_local $0
          get_local $3
          i32.store offset=40
          get_local $0
          get_local $3
          i32.store offset=44
          get_local $0
          get_local $3
          i32.store offset=48
          get_local $0
          get_local $3
          i32.store offset=52
          get_local $0
          get_local $3
          i32.store offset=56
          get_local $0
          get_local $3
          i32.store offset=60
          get_local $0
          i32.const -64
          i32.sub
          set_local $0
          br $loop_0
        end ;; $if_1
      end ;; $loop_0
      loop $loop_1
        get_local $0
        get_local $5
        i32.lt_s
        if $if_2
          get_local $0
          get_local $3
          i32.store
          get_local $0
          i32.const 4
          i32.add
          set_local $0
          br $loop_1
        end ;; $if_2
      end ;; $loop_1
    end ;; $if
    loop $loop_2
      get_local $0
      get_local $4
      i32.lt_s
      if $if_3
        get_local $0
        get_local $1
        i32.store8
        get_local $0
        i32.const 1
        i32.add
        set_local $0
        br $loop_2
      end ;; $if_3
    end ;; $loop_2
    get_local $4
    get_local $2
    i32.sub
    )
  
  (func $_sbrk (type $5)
    (param $0 i32)
    (result i32)
    (local $1 i32)
    (local $2 i32)
    get_local $0
    get_global $14
    i32.load
    tee_local $1
    i32.add
    tee_local $2
    get_local $1
    i32.lt_s
    get_local $0
    i32.const 0
    i32.gt_s
    i32.and
    get_local $2
    i32.const 0
    i32.lt_s
    i32.or
    if $if
      call $abortOnCannotGrowMemory
      drop
      i32.const 12
      call $___setErrNo
      i32.const -1
      return
    end ;; $if
    get_global $14
    get_local $2
    i32.store
    get_local $2
    call $getTotalMemory
    i32.gt_s
    if $if_0
      call $enlargeMemory
      i32.eqz
      if $if_1
        get_global $14
        get_local $1
        i32.store
        i32.const 12
        call $___setErrNo
        i32.const -1
        return
      end ;; $if_1
    end ;; $if_0
    get_local $1
    )
  
  (func $dynCall_v (type $1)
    (param $0 i32)
    i32.const 0
    call_indirect $10 (type $0)
    )
  
  (func $b0 (type $0)
    i32.const 0
    call $abort
    ))