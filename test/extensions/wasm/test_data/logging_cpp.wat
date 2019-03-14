(module
  (type $0 (func))
  (type $1 (func (param i32)))
  (type $2 (func (result i32)))
  (type $3 (func (param i32 i32 i32) (result i32)))
  (type $4 (func (param i32) (result i32)))
  (type $5 (func (param i32 i32 i32)))
  (type $6 (func (param i32 i32)))
  (type $7 (func (param i32 i32 i32 i32 i32 i32 i32)))
  (type $8 (func (param i32 i32 i32 i32) (result i32)))
  (import "env" "abort" (func $abort (param i32)))
  (import "env" "abortOnCannotGrowMemory" (func $abortOnCannotGrowMemory (result i32)))
  (import "env" "___setErrNo" (func $___setErrNo (param i32)))
  (import "env" "_abort" (func $_abort))
  (import "env" "_emscripten_get_heap_size" (func $_emscripten_get_heap_size (result i32)))
  (import "env" "_emscripten_memcpy_big" (func $_emscripten_memcpy_big (param i32 i32 i32) (result i32)))
  (import "env" "_emscripten_resize_heap" (func $_emscripten_resize_heap (param i32) (result i32)))
  (import "env" "_proxy_log" (func $_proxy_log (param i32 i32 i32)))
  (import "env" "table" (table $9 1 1 funcref))
  (import "env" "memory" (memory $10 256 256))
  (import "env" "__table_base" (global $11 i32))
  (import "env" "DYNAMICTOP_PTR" (global $12 i32))
  (export "___errno_location" (func $___errno_location))
  (export "_free" (func $_free))
  (export "_malloc" (func $_malloc))
  (export "_memcpy" (func $_memcpy))
  (export "_memmove" (func $_memmove))
  (export "_memset" (func $_memset))
  (export "_proxy_onConfigure" (func $_proxy_onConfigure))
  (export "_proxy_onStart" (func $_proxy_onStart))
  (export "_proxy_onTick" (func $_proxy_onTick))
  (export "_sbrk" (func $_sbrk))
  (export "dynCall_v" (func $dynCall_v))
  (export "establishStackSpace" (func $establishStackSpace))
  (export "setThrew" (func $setThrew))
  (export "stackAlloc" (func $stackAlloc))
  (export "stackRestore" (func $stackRestore))
  (export "stackSave" (func $stackSave))
  (global $13  (mut i32) (global.get $12))
  (global $14  (mut i32) (i32.const 0))
  (global $15  (mut i32) (i32.const 0))
  (global $16  (mut i32) (i32.const 2864))
  (global $17  (mut i32) (i32.const 5245744))
  (elem $9 (global.get $11)
    $b0)
  (data $10 (i32.const 1024)
    "warn \00test debug\00 logging\00test info\00test tick")
  
  (func $stackAlloc (type $4)
    (param $0 i32)
    (result i32)
    (local $1 i32)
    global.get $16
    local.set $1
    local.get $0
    global.get $16
    i32.add
    global.set $16
    global.get $16
    i32.const 15
    i32.add
    i32.const -16
    i32.and
    global.set $16
    local.get $1
    )
  
  (func $stackSave (type $2)
    (result i32)
    global.get $16
    )
  
  (func $stackRestore (type $1)
    (param $0 i32)
    local.get $0
    global.set $16
    )
  
  (func $establishStackSpace (type $6)
    (param $0 i32)
    (param $1 i32)
    local.get $0
    global.set $16
    local.get $1
    global.set $17
    )
  
  (func $setThrew (type $6)
    (param $0 i32)
    (param $1 i32)
    global.get $14
    i32.eqz
    if $if
      local.get $0
      global.set $14
      local.get $1
      global.set $15
    end ;; $if
    )
  
  (func $_proxy_onConfigure (type $6)
    (param $0 i32)
    (param $1 i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    global.get $16
    local.set $5
    global.get $16
    i32.const 32
    i32.add
    global.set $16
    local.get $5
    local.tee $2
    i64.const 0
    i64.store align=4
    local.get $2
    i32.const 0
    i32.store offset=8
    local.get $1
    i32.const -17
    i32.gt_u
    if $if
      call $__ZNKSt3__221__basic_string_commonILb1EE20__throw_length_errorEv
    end ;; $if
    local.get $5
    i32.const 12
    i32.add
    local.set $3
    block $block
      block $block_0
        local.get $1
        i32.const 11
        i32.lt_u
        if $if_0 (result i32)
          local.get $2
          local.get $1
          i32.store8 offset=11
          local.get $1
          if $if_1 (result i32)
            local.get $2
            local.set $4
            br $block_0
          else
            local.get $2
          end ;; $if_1
        else
          local.get $2
          local.get $1
          i32.const 16
          i32.add
          i32.const -16
          i32.and
          local.tee $6
          call $__Znwm
          local.tee $4
          i32.store
          local.get $2
          local.get $6
          i32.const -2147483648
          i32.or
          i32.store offset=8
          local.get $2
          local.get $1
          i32.store offset=4
          br $block_0
        end ;; $if_0
        local.set $4
        br $block
      end ;; $block_0
      local.get $4
      local.get $0
      local.get $1
      call $_memcpy
      drop
    end ;; $block
    local.get $1
    local.get $4
    i32.add
    i32.const 0
    i32.store8
    local.get $3
    local.get $2
    call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6insertEmPKc
    local.tee $0
    i64.load align=4
    i64.store align=4
    local.get $3
    local.get $0
    i32.load offset=8
    i32.store offset=8
    local.get $0
    i64.const 0
    i64.store align=4
    local.get $0
    i32.const 0
    i32.store offset=8
    i32.const 3
    local.get $3
    i32.load
    local.get $3
    local.get $3
    i32.const 11
    i32.add
    local.tee $4
    i32.load8_s
    local.tee $1
    i32.const 0
    i32.lt_s
    local.tee $0
    select
    local.get $3
    i32.load offset=4
    local.get $1
    i32.const 255
    i32.and
    local.get $0
    select
    call $_proxy_log
    local.get $4
    i32.load8_s
    i32.const 0
    i32.lt_s
    if $if_2
      local.get $3
      i32.load
      call $__ZdlPv
    end ;; $if_2
    local.get $2
    i32.load8_s offset=11
    i32.const 0
    i32.ge_s
    if $if_3
      local.get $5
      global.set $16
      return
    end ;; $if_3
    local.get $2
    i32.load
    call $__ZdlPv
    local.get $5
    global.set $16
    )
  
  (func $_proxy_onStart (type $0)
    (local $0 i32)
    (local $1 i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    global.get $16
    local.set $0
    global.get $16
    i32.const 32
    i32.add
    global.set $16
    local.get $0
    i32.const 11
    i32.add
    local.tee $3
    i32.const 10
    i32.store8
    local.get $0
    i32.const 1030
    i64.load align=1
    i64.store align=1
    local.get $0
    i32.const 1038
    i32.load16_s align=1
    i32.store16 offset=8 align=1
    local.get $0
    i32.const 0
    i32.store8 offset=10
    local.get $0
    i32.const 12
    i32.add
    local.tee $1
    local.get $0
    call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6appendEPKc
    local.tee $2
    i64.load align=4
    i64.store align=4
    local.get $1
    local.get $2
    i32.load offset=8
    i32.store offset=8
    local.get $2
    i64.const 0
    i64.store align=4
    local.get $2
    i32.const 0
    i32.store offset=8
    i32.const 1
    local.get $1
    i32.load
    local.get $1
    local.get $1
    i32.const 11
    i32.add
    local.tee $4
    i32.load8_s
    local.tee $5
    i32.const 0
    i32.lt_s
    local.tee $2
    select
    local.get $1
    i32.load offset=4
    local.get $5
    i32.const 255
    i32.and
    local.get $2
    select
    call $_proxy_log
    local.get $4
    i32.load8_s
    i32.const 0
    i32.lt_s
    if $if
      local.get $1
      i32.load
      call $__ZdlPv
    end ;; $if
    local.get $3
    i32.load8_s
    i32.const 0
    i32.lt_s
    if $if_0
      local.get $0
      i32.load
      call $__ZdlPv
    end ;; $if_0
    local.get $0
    i64.const 0
    i64.store align=4
    local.get $0
    i32.const 0
    i32.store offset=8
    local.get $0
    i32.const 11
    i32.add
    local.tee $3
    i32.const 9
    i32.store8
    local.get $0
    i32.const 1050
    i64.load align=1
    i64.store align=1
    local.get $0
    i32.const 1058
    i32.load8_s
    i32.store8 offset=8
    local.get $0
    i32.const 0
    i32.store8 offset=9
    local.get $1
    local.get $0
    call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6appendEPKc
    local.tee $2
    i64.load align=4
    i64.store align=4
    local.get $1
    local.get $2
    i32.load offset=8
    i32.store offset=8
    local.get $2
    i64.const 0
    i64.store align=4
    local.get $2
    i32.const 0
    i32.store offset=8
    i32.const 2
    local.get $1
    i32.load
    local.get $1
    local.get $1
    i32.const 11
    i32.add
    local.tee $4
    i32.load8_s
    local.tee $5
    i32.const 0
    i32.lt_s
    local.tee $2
    select
    local.get $1
    i32.load offset=4
    local.get $5
    i32.const 255
    i32.and
    local.get $2
    select
    call $_proxy_log
    local.get $4
    i32.load8_s
    i32.const 0
    i32.lt_s
    if $if_1
      local.get $1
      i32.load
      call $__ZdlPv
    end ;; $if_1
    local.get $3
    i32.load8_s
    i32.const 0
    i32.ge_s
    if $if_2
      local.get $0
      global.set $16
      return
    end ;; $if_2
    local.get $0
    i32.load
    call $__ZdlPv
    local.get $0
    global.set $16
    )
  
  (func $_proxy_onTick (type $0)
    (local $0 i32)
    (local $1 i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    global.get $16
    local.set $0
    global.get $16
    i32.const 32
    i32.add
    global.set $16
    local.get $0
    i64.const 0
    i64.store align=4
    local.get $0
    i32.const 0
    i32.store offset=8
    local.get $0
    i32.const 11
    i32.add
    local.tee $3
    i32.const 9
    i32.store8
    local.get $0
    i32.const 1060
    i64.load align=1
    i64.store align=1
    local.get $0
    i32.const 1068
    i32.load8_s
    i32.store8 offset=8
    local.get $0
    i32.const 0
    i32.store8 offset=9
    local.get $0
    i32.const 12
    i32.add
    local.tee $1
    local.get $0
    call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6appendEPKc
    local.tee $2
    i64.load align=4
    i64.store align=4
    local.get $1
    local.get $2
    i32.load offset=8
    i32.store offset=8
    local.get $2
    i64.const 0
    i64.store align=4
    local.get $2
    i32.const 0
    i32.store offset=8
    i32.const 4
    local.get $1
    i32.load
    local.get $1
    local.get $1
    i32.const 11
    i32.add
    local.tee $4
    i32.load8_s
    local.tee $5
    i32.const 0
    i32.lt_s
    local.tee $2
    select
    local.get $1
    i32.load offset=4
    local.get $5
    i32.const 255
    i32.and
    local.get $2
    select
    call $_proxy_log
    local.get $4
    i32.load8_s
    i32.const 0
    i32.lt_s
    if $if
      local.get $1
      i32.load
      call $__ZdlPv
    end ;; $if
    local.get $3
    i32.load8_s
    i32.const 0
    i32.ge_s
    if $if_0
      local.get $0
      global.set $16
      return
    end ;; $if_0
    local.get $0
    i32.load
    call $__ZdlPv
    local.get $0
    global.set $16
    )
  
  (func $___errno_location (type $2)
    (result i32)
    i32.const 1072
    )
  
  (func $_strlen (type $4)
    (param $0 i32)
    (result i32)
    (local $1 i32)
    (local $2 i32)
    (local $3 i32)
    block $block
      block $block_0
        local.get $0
        local.tee $2
        i32.const 3
        i32.and
        i32.eqz
        br_if $block_0
        local.get $2
        local.tee $1
        local.set $0
        block $block_1
          loop $loop
            local.get $1
            i32.load8_s
            i32.eqz
            br_if $block_1
            local.get $1
            i32.const 1
            i32.add
            local.tee $1
            local.tee $0
            i32.const 3
            i32.and
            br_if $loop
          end ;; $loop
          local.get $1
          local.set $0
          br $block_0
        end ;; $block_1
        br $block
      end ;; $block_0
      loop $loop_0
        local.get $0
        i32.const 4
        i32.add
        local.set $1
        local.get $0
        i32.load
        local.tee $3
        i32.const -16843009
        i32.add
        local.get $3
        i32.const -2139062144
        i32.and
        i32.const -2139062144
        i32.xor
        i32.and
        i32.eqz
        if $if
          local.get $1
          local.set $0
          br $loop_0
        end ;; $if
      end ;; $loop_0
      local.get $3
      i32.const 255
      i32.and
      if $if_0
        loop $loop_1
          local.get $0
          i32.const 1
          i32.add
          local.tee $0
          i32.load8_s
          br_if $loop_1
        end ;; $loop_1
      end ;; $if_0
    end ;; $block
    local.get $0
    local.get $2
    i32.sub
    )
  
  (func $_malloc (type $4)
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
    global.get $16
    local.set $14
    global.get $16
    i32.const 16
    i32.add
    global.set $16
    local.get $0
    i32.const 245
    i32.lt_u
    if $if (result i32)
      i32.const 1076
      i32.load
      local.tee $7
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
      local.tee $2
      i32.const 3
      i32.shr_u
      local.tee $0
      i32.shr_u
      local.tee $3
      i32.const 3
      i32.and
      if $if_0
        local.get $3
        i32.const 1
        i32.and
        i32.const 1
        i32.xor
        local.get $0
        i32.add
        local.tee $1
        i32.const 3
        i32.shl
        i32.const 1116
        i32.add
        local.tee $2
        i32.const 8
        i32.add
        local.tee $4
        i32.load
        local.tee $3
        i32.const 8
        i32.add
        local.tee $6
        i32.load
        local.set $0
        local.get $0
        local.get $2
        i32.eq
        if $if_1
          i32.const 1076
          i32.const 1
          local.get $1
          i32.shl
          i32.const -1
          i32.xor
          local.get $7
          i32.and
          i32.store
        else
          i32.const 1092
          i32.load
          local.get $0
          i32.gt_u
          if $if_2
            call $_abort
          end ;; $if_2
          local.get $3
          local.get $0
          i32.const 12
          i32.add
          local.tee $5
          i32.load
          i32.eq
          if $if_3
            local.get $5
            local.get $2
            i32.store
            local.get $4
            local.get $0
            i32.store
          else
            call $_abort
          end ;; $if_3
        end ;; $if_1
        local.get $3
        local.get $1
        i32.const 3
        i32.shl
        local.tee $0
        i32.const 3
        i32.or
        i32.store offset=4
        local.get $0
        local.get $3
        i32.add
        i32.const 4
        i32.add
        local.tee $0
        local.get $0
        i32.load
        i32.const 1
        i32.or
        i32.store
        local.get $14
        global.set $16
        local.get $6
        return
      end ;; $if_0
      local.get $2
      i32.const 1084
      i32.load
      local.tee $13
      i32.gt_u
      if $if_4 (result i32)
        local.get $3
        if $if_5
          local.get $3
          local.get $0
          i32.shl
          i32.const 2
          local.get $0
          i32.shl
          local.tee $0
          i32.const 0
          local.get $0
          i32.sub
          i32.or
          i32.and
          local.tee $0
          i32.const 0
          local.get $0
          i32.sub
          i32.and
          i32.const -1
          i32.add
          local.tee $0
          i32.const 12
          i32.shr_u
          i32.const 16
          i32.and
          local.tee $3
          local.get $0
          local.get $3
          i32.shr_u
          local.tee $0
          i32.const 5
          i32.shr_u
          i32.const 8
          i32.and
          local.tee $3
          i32.or
          local.get $0
          local.get $3
          i32.shr_u
          local.tee $0
          i32.const 2
          i32.shr_u
          i32.const 4
          i32.and
          local.tee $3
          i32.or
          local.get $0
          local.get $3
          i32.shr_u
          local.tee $0
          i32.const 1
          i32.shr_u
          i32.const 2
          i32.and
          local.tee $3
          i32.or
          local.get $0
          local.get $3
          i32.shr_u
          local.tee $0
          i32.const 1
          i32.shr_u
          i32.const 1
          i32.and
          local.tee $3
          i32.or
          local.get $0
          local.get $3
          i32.shr_u
          i32.add
          local.tee $1
          i32.const 3
          i32.shl
          i32.const 1116
          i32.add
          local.tee $5
          i32.const 8
          i32.add
          local.tee $9
          i32.load
          local.tee $0
          i32.const 8
          i32.add
          local.tee $10
          i32.load
          local.set $3
          local.get $3
          local.get $5
          i32.eq
          if $if_6
            i32.const 1076
            i32.const 1
            local.get $1
            i32.shl
            i32.const -1
            i32.xor
            local.get $7
            i32.and
            local.tee $4
            i32.store
          else
            i32.const 1092
            i32.load
            local.get $3
            i32.gt_u
            if $if_7
              call $_abort
            end ;; $if_7
            local.get $0
            local.get $3
            i32.const 12
            i32.add
            local.tee $11
            i32.load
            i32.eq
            if $if_8
              local.get $11
              local.get $5
              i32.store
              local.get $9
              local.get $3
              i32.store
              local.get $7
              local.set $4
            else
              call $_abort
            end ;; $if_8
          end ;; $if_6
          local.get $0
          local.get $2
          i32.const 3
          i32.or
          i32.store offset=4
          local.get $0
          local.get $2
          i32.add
          local.tee $7
          local.get $1
          i32.const 3
          i32.shl
          local.tee $3
          local.get $2
          i32.sub
          local.tee $5
          i32.const 1
          i32.or
          i32.store offset=4
          local.get $0
          local.get $3
          i32.add
          local.get $5
          i32.store
          local.get $13
          if $if_9
            i32.const 1096
            i32.load
            local.set $2
            local.get $13
            i32.const 3
            i32.shr_u
            local.tee $3
            i32.const 3
            i32.shl
            i32.const 1116
            i32.add
            local.set $0
            i32.const 1
            local.get $3
            i32.shl
            local.tee $3
            local.get $4
            i32.and
            if $if_10
              i32.const 1092
              i32.load
              local.get $0
              i32.const 8
              i32.add
              local.tee $3
              i32.load
              local.tee $1
              i32.gt_u
              if $if_11
                call $_abort
              else
                local.get $1
                local.set $6
                local.get $3
                local.set $12
              end ;; $if_11
            else
              i32.const 1076
              local.get $3
              local.get $4
              i32.or
              i32.store
              local.get $0
              local.set $6
              local.get $0
              i32.const 8
              i32.add
              local.set $12
            end ;; $if_10
            local.get $12
            local.get $2
            i32.store
            local.get $6
            local.get $2
            i32.store offset=12
            local.get $2
            local.get $6
            i32.store offset=8
            local.get $2
            local.get $0
            i32.store offset=12
          end ;; $if_9
          i32.const 1084
          local.get $5
          i32.store
          i32.const 1096
          local.get $7
          i32.store
          local.get $14
          global.set $16
          local.get $10
          return
        end ;; $if_5
        i32.const 1080
        i32.load
        local.tee $12
        if $if_12 (result i32)
          local.get $12
          i32.const 0
          local.get $12
          i32.sub
          i32.and
          i32.const -1
          i32.add
          local.tee $0
          i32.const 12
          i32.shr_u
          i32.const 16
          i32.and
          local.tee $3
          local.get $0
          local.get $3
          i32.shr_u
          local.tee $0
          i32.const 5
          i32.shr_u
          i32.const 8
          i32.and
          local.tee $3
          i32.or
          local.get $0
          local.get $3
          i32.shr_u
          local.tee $0
          i32.const 2
          i32.shr_u
          i32.const 4
          i32.and
          local.tee $3
          i32.or
          local.get $0
          local.get $3
          i32.shr_u
          local.tee $0
          i32.const 1
          i32.shr_u
          i32.const 2
          i32.and
          local.tee $3
          i32.or
          local.get $0
          local.get $3
          i32.shr_u
          local.tee $0
          i32.const 1
          i32.shr_u
          i32.const 1
          i32.and
          local.tee $3
          i32.or
          local.get $0
          local.get $3
          i32.shr_u
          i32.add
          i32.const 2
          i32.shl
          i32.const 1380
          i32.add
          i32.load
          local.tee $4
          local.set $0
          local.get $4
          i32.load offset=4
          i32.const -8
          i32.and
          local.get $2
          i32.sub
          local.set $10
          loop $loop
            block $block
              local.get $0
              i32.load offset=16
              local.tee $3
              if $if_13
                local.get $3
                local.set $0
              else
                local.get $0
                i32.load offset=20
                local.tee $0
                i32.eqz
                br_if $block
              end ;; $if_13
              local.get $0
              local.get $4
              local.get $0
              i32.load offset=4
              i32.const -8
              i32.and
              local.get $2
              i32.sub
              local.tee $3
              local.get $10
              i32.lt_u
              local.tee $6
              select
              local.set $4
              local.get $3
              local.get $10
              local.get $6
              select
              local.set $10
              br $loop
            end ;; $block
          end ;; $loop
          i32.const 1092
          i32.load
          local.tee $15
          local.get $4
          i32.gt_u
          if $if_14
            call $_abort
          end ;; $if_14
          local.get $2
          local.get $4
          i32.add
          local.tee $8
          local.get $4
          i32.le_u
          if $if_15
            call $_abort
          end ;; $if_15
          local.get $4
          i32.load offset=24
          local.set $11
          local.get $4
          local.get $4
          i32.load offset=12
          local.tee $0
          i32.eq
          if $if_16
            block $block_0
              local.get $4
              i32.const 20
              i32.add
              local.tee $3
              i32.load
              local.tee $0
              i32.eqz
              if $if_17
                local.get $4
                i32.const 16
                i32.add
                local.tee $3
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
                  local.tee $6
                  i32.load
                  local.tee $9
                  i32.eqz
                  if $if_18
                    local.get $0
                    i32.const 16
                    i32.add
                    local.tee $6
                    i32.load
                    local.tee $9
                    i32.eqz
                    br_if $block_1
                  end ;; $if_18
                  local.get $6
                  local.set $3
                  local.get $9
                  local.set $0
                  br $loop_0
                end ;; $block_1
              end ;; $loop_0
              local.get $15
              local.get $3
              i32.gt_u
              if $if_19
                call $_abort
              else
                local.get $3
                i32.const 0
                i32.store
                local.get $0
                local.set $1
              end ;; $if_19
            end ;; $block_0
          else
            local.get $15
            local.get $4
            i32.load offset=8
            local.tee $3
            i32.gt_u
            if $if_20
              call $_abort
            end ;; $if_20
            local.get $3
            i32.const 12
            i32.add
            local.tee $6
            i32.load
            local.get $4
            i32.ne
            if $if_21
              call $_abort
            end ;; $if_21
            local.get $4
            local.get $0
            i32.const 8
            i32.add
            local.tee $9
            i32.load
            i32.eq
            if $if_22
              local.get $6
              local.get $0
              i32.store
              local.get $9
              local.get $3
              i32.store
              local.get $0
              local.set $1
            else
              call $_abort
            end ;; $if_22
          end ;; $if_16
          local.get $11
          if $if_23
            block $block_2
              local.get $4
              local.get $4
              i32.load offset=28
              local.tee $0
              i32.const 2
              i32.shl
              i32.const 1380
              i32.add
              local.tee $3
              i32.load
              i32.eq
              if $if_24
                local.get $3
                local.get $1
                i32.store
                local.get $1
                i32.eqz
                if $if_25
                  i32.const 1080
                  local.get $12
                  i32.const 1
                  local.get $0
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
                local.get $11
                i32.gt_u
                if $if_26
                  call $_abort
                else
                  local.get $11
                  i32.const 16
                  i32.add
                  local.tee $0
                  local.get $11
                  i32.const 20
                  i32.add
                  local.get $4
                  local.get $0
                  i32.load
                  i32.eq
                  select
                  local.get $1
                  i32.store
                  local.get $1
                  i32.eqz
                  br_if $block_2
                end ;; $if_26
              end ;; $if_24
              i32.const 1092
              i32.load
              local.tee $3
              local.get $1
              i32.gt_u
              if $if_27
                call $_abort
              end ;; $if_27
              local.get $1
              local.get $11
              i32.store offset=24
              local.get $4
              i32.load offset=16
              local.tee $0
              if $if_28
                local.get $3
                local.get $0
                i32.gt_u
                if $if_29
                  call $_abort
                else
                  local.get $1
                  local.get $0
                  i32.store offset=16
                  local.get $0
                  local.get $1
                  i32.store offset=24
                end ;; $if_29
              end ;; $if_28
              local.get $4
              i32.load offset=20
              local.tee $0
              if $if_30
                i32.const 1092
                i32.load
                local.get $0
                i32.gt_u
                if $if_31
                  call $_abort
                else
                  local.get $1
                  local.get $0
                  i32.store offset=20
                  local.get $0
                  local.get $1
                  i32.store offset=24
                end ;; $if_31
              end ;; $if_30
            end ;; $block_2
          end ;; $if_23
          local.get $10
          i32.const 16
          i32.lt_u
          if $if_32
            local.get $4
            local.get $2
            local.get $10
            i32.add
            local.tee $0
            i32.const 3
            i32.or
            i32.store offset=4
            local.get $0
            local.get $4
            i32.add
            i32.const 4
            i32.add
            local.tee $0
            local.get $0
            i32.load
            i32.const 1
            i32.or
            i32.store
          else
            local.get $4
            local.get $2
            i32.const 3
            i32.or
            i32.store offset=4
            local.get $8
            local.get $10
            i32.const 1
            i32.or
            i32.store offset=4
            local.get $8
            local.get $10
            i32.add
            local.get $10
            i32.store
            local.get $13
            if $if_33
              i32.const 1096
              i32.load
              local.set $2
              local.get $13
              i32.const 3
              i32.shr_u
              local.tee $3
              i32.const 3
              i32.shl
              i32.const 1116
              i32.add
              local.set $0
              i32.const 1
              local.get $3
              i32.shl
              local.tee $3
              local.get $7
              i32.and
              if $if_34
                i32.const 1092
                i32.load
                local.get $0
                i32.const 8
                i32.add
                local.tee $3
                i32.load
                local.tee $1
                i32.gt_u
                if $if_35
                  call $_abort
                else
                  local.get $1
                  local.set $5
                  local.get $3
                  local.set $16
                end ;; $if_35
              else
                i32.const 1076
                local.get $3
                local.get $7
                i32.or
                i32.store
                local.get $0
                local.set $5
                local.get $0
                i32.const 8
                i32.add
                local.set $16
              end ;; $if_34
              local.get $16
              local.get $2
              i32.store
              local.get $5
              local.get $2
              i32.store offset=12
              local.get $2
              local.get $5
              i32.store offset=8
              local.get $2
              local.get $0
              i32.store offset=12
            end ;; $if_33
            i32.const 1084
            local.get $10
            i32.store
            i32.const 1096
            local.get $8
            i32.store
          end ;; $if_32
          local.get $14
          global.set $16
          local.get $4
          i32.const 8
          i32.add
          return
        else
          local.get $2
        end ;; $if_12
      else
        local.get $2
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
          local.set $4
          i32.const 1080
          i32.load
          local.tee $6
          if $if_37 (result i32)
            local.get $0
            i32.const 8
            i32.shr_u
            local.tee $0
            if $if_38 (result i32)
              local.get $4
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
                local.tee $1
                i32.shl
                local.tee $2
                i32.const 520192
                i32.add
                i32.const 16
                i32.shr_u
                i32.const 4
                i32.and
                local.set $0
                i32.const 14
                local.get $0
                local.get $1
                i32.or
                local.get $2
                local.get $0
                i32.shl
                local.tee $0
                i32.const 245760
                i32.add
                i32.const 16
                i32.shr_u
                i32.const 2
                i32.and
                local.tee $1
                i32.or
                i32.sub
                local.get $0
                local.get $1
                i32.shl
                i32.const 15
                i32.shr_u
                i32.add
                local.tee $0
                i32.const 1
                i32.shl
                local.get $4
                local.get $0
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
            local.set $18
            i32.const 0
            local.get $4
            i32.sub
            local.set $2
            block $block_4
              block $block_5
                local.get $18
                i32.const 2
                i32.shl
                i32.const 1380
                i32.add
                i32.load
                local.tee $0
                if $if_40 (result i32)
                  i32.const 0
                  local.set $1
                  local.get $4
                  i32.const 0
                  i32.const 25
                  local.get $18
                  i32.const 1
                  i32.shr_u
                  i32.sub
                  local.get $18
                  i32.const 31
                  i32.eq
                  select
                  i32.shl
                  local.set $12
                  loop $loop_1 (result i32)
                    local.get $0
                    i32.load offset=4
                    i32.const -8
                    i32.and
                    local.get $4
                    i32.sub
                    local.tee $16
                    local.get $2
                    i32.lt_u
                    if $if_41
                      local.get $16
                      if $if_42 (result i32)
                        local.get $16
                        local.set $2
                        local.get $0
                      else
                        i32.const 0
                        local.set $2
                        local.get $0
                        local.set $1
                        br $block_5
                      end ;; $if_42
                      local.set $1
                    end ;; $if_41
                    local.get $5
                    local.get $0
                    i32.load offset=20
                    local.tee $5
                    local.get $5
                    i32.eqz
                    local.get $5
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
                    local.tee $0
                    i32.eq
                    i32.or
                    select
                    local.set $5
                    local.get $12
                    i32.const 1
                    i32.shl
                    local.set $12
                    local.get $0
                    br_if $loop_1
                    local.get $1
                  end ;; $loop_1
                else
                  i32.const 0
                end ;; $if_40
                local.set $0
                local.get $0
                local.get $5
                i32.or
                if $if_43 (result i32)
                  local.get $5
                else
                  local.get $4
                  local.get $6
                  i32.const 2
                  local.get $18
                  i32.shl
                  local.tee $0
                  i32.const 0
                  local.get $0
                  i32.sub
                  i32.or
                  i32.and
                  local.tee $1
                  i32.eqz
                  br_if $block_3
                  drop
                  i32.const 0
                  local.set $0
                  local.get $1
                  i32.const 0
                  local.get $1
                  i32.sub
                  i32.and
                  i32.const -1
                  i32.add
                  local.tee $1
                  i32.const 12
                  i32.shr_u
                  i32.const 16
                  i32.and
                  local.tee $5
                  local.get $1
                  local.get $5
                  i32.shr_u
                  local.tee $1
                  i32.const 5
                  i32.shr_u
                  i32.const 8
                  i32.and
                  local.tee $5
                  i32.or
                  local.get $1
                  local.get $5
                  i32.shr_u
                  local.tee $1
                  i32.const 2
                  i32.shr_u
                  i32.const 4
                  i32.and
                  local.tee $5
                  i32.or
                  local.get $1
                  local.get $5
                  i32.shr_u
                  local.tee $1
                  i32.const 1
                  i32.shr_u
                  i32.const 2
                  i32.and
                  local.tee $5
                  i32.or
                  local.get $1
                  local.get $5
                  i32.shr_u
                  local.tee $1
                  i32.const 1
                  i32.shr_u
                  i32.const 1
                  i32.and
                  local.tee $5
                  i32.or
                  local.get $1
                  local.get $5
                  i32.shr_u
                  i32.add
                  i32.const 2
                  i32.shl
                  i32.const 1380
                  i32.add
                  i32.load
                end ;; $if_43
                local.tee $1
                br_if $block_5
                local.get $0
                local.set $5
                br $block_4
              end ;; $block_5
              local.get $0
              local.set $5
              local.get $2
              local.set $0
              loop $loop_2 (result i32)
                local.get $1
                i32.load offset=4
                local.set $12
                local.get $1
                i32.load offset=16
                local.tee $2
                i32.eqz
                if $if_44
                  local.get $1
                  i32.load offset=20
                  local.set $2
                end ;; $if_44
                local.get $12
                i32.const -8
                i32.and
                local.get $4
                i32.sub
                local.tee $16
                local.get $0
                i32.lt_u
                local.set $12
                local.get $16
                local.get $0
                local.get $12
                select
                local.set $0
                local.get $1
                local.get $5
                local.get $12
                select
                local.set $5
                local.get $2
                if $if_45 (result i32)
                  local.get $2
                  local.set $1
                  br $loop_2
                else
                  local.get $0
                end ;; $if_45
              end ;; $loop_2
              local.set $2
            end ;; $block_4
            local.get $5
            if $if_46 (result i32)
              local.get $2
              i32.const 1084
              i32.load
              local.get $4
              i32.sub
              i32.lt_u
              if $if_47 (result i32)
                i32.const 1092
                i32.load
                local.tee $17
                local.get $5
                i32.gt_u
                if $if_48
                  call $_abort
                end ;; $if_48
                local.get $4
                local.get $5
                i32.add
                local.tee $8
                local.get $5
                i32.le_u
                if $if_49
                  call $_abort
                end ;; $if_49
                local.get $5
                i32.load offset=24
                local.set $15
                local.get $5
                local.get $5
                i32.load offset=12
                local.tee $0
                i32.eq
                if $if_50
                  block $block_6
                    local.get $5
                    i32.const 20
                    i32.add
                    local.tee $1
                    i32.load
                    local.tee $0
                    i32.eqz
                    if $if_51
                      local.get $5
                      i32.const 16
                      i32.add
                      local.tee $1
                      i32.load
                      local.tee $0
                      i32.eqz
                      br_if $block_6
                    end ;; $if_51
                    loop $loop_3
                      block $block_7
                        local.get $0
                        i32.const 20
                        i32.add
                        local.tee $9
                        i32.load
                        local.tee $11
                        i32.eqz
                        if $if_52
                          local.get $0
                          i32.const 16
                          i32.add
                          local.tee $9
                          i32.load
                          local.tee $11
                          i32.eqz
                          br_if $block_7
                        end ;; $if_52
                        local.get $9
                        local.set $1
                        local.get $11
                        local.set $0
                        br $loop_3
                      end ;; $block_7
                    end ;; $loop_3
                    local.get $17
                    local.get $1
                    i32.gt_u
                    if $if_53
                      call $_abort
                    else
                      local.get $1
                      i32.const 0
                      i32.store
                      local.get $0
                      local.set $7
                    end ;; $if_53
                  end ;; $block_6
                else
                  local.get $17
                  local.get $5
                  i32.load offset=8
                  local.tee $1
                  i32.gt_u
                  if $if_54
                    call $_abort
                  end ;; $if_54
                  local.get $1
                  i32.const 12
                  i32.add
                  local.tee $9
                  i32.load
                  local.get $5
                  i32.ne
                  if $if_55
                    call $_abort
                  end ;; $if_55
                  local.get $5
                  local.get $0
                  i32.const 8
                  i32.add
                  local.tee $11
                  i32.load
                  i32.eq
                  if $if_56
                    local.get $9
                    local.get $0
                    i32.store
                    local.get $11
                    local.get $1
                    i32.store
                    local.get $0
                    local.set $7
                  else
                    call $_abort
                  end ;; $if_56
                end ;; $if_50
                local.get $15
                if $if_57
                  block $block_8
                    local.get $5
                    local.get $5
                    i32.load offset=28
                    local.tee $0
                    i32.const 2
                    i32.shl
                    i32.const 1380
                    i32.add
                    local.tee $1
                    i32.load
                    i32.eq
                    if $if_58
                      local.get $1
                      local.get $7
                      i32.store
                      local.get $7
                      i32.eqz
                      if $if_59
                        i32.const 1080
                        local.get $6
                        i32.const 1
                        local.get $0
                        i32.shl
                        i32.const -1
                        i32.xor
                        i32.and
                        local.tee $3
                        i32.store
                        br $block_8
                      end ;; $if_59
                    else
                      i32.const 1092
                      i32.load
                      local.get $15
                      i32.gt_u
                      if $if_60
                        call $_abort
                      else
                        local.get $15
                        i32.const 16
                        i32.add
                        local.tee $0
                        local.get $15
                        i32.const 20
                        i32.add
                        local.get $5
                        local.get $0
                        i32.load
                        i32.eq
                        select
                        local.get $7
                        i32.store
                        local.get $7
                        i32.eqz
                        if $if_61
                          local.get $6
                          local.set $3
                          br $block_8
                        end ;; $if_61
                      end ;; $if_60
                    end ;; $if_58
                    i32.const 1092
                    i32.load
                    local.tee $1
                    local.get $7
                    i32.gt_u
                    if $if_62
                      call $_abort
                    end ;; $if_62
                    local.get $7
                    local.get $15
                    i32.store offset=24
                    local.get $5
                    i32.load offset=16
                    local.tee $0
                    if $if_63
                      local.get $1
                      local.get $0
                      i32.gt_u
                      if $if_64
                        call $_abort
                      else
                        local.get $7
                        local.get $0
                        i32.store offset=16
                        local.get $0
                        local.get $7
                        i32.store offset=24
                      end ;; $if_64
                    end ;; $if_63
                    local.get $5
                    i32.load offset=20
                    local.tee $0
                    if $if_65
                      i32.const 1092
                      i32.load
                      local.get $0
                      i32.gt_u
                      if $if_66
                        call $_abort
                      else
                        local.get $7
                        local.get $0
                        i32.store offset=20
                        local.get $0
                        local.get $7
                        i32.store offset=24
                        local.get $6
                        local.set $3
                      end ;; $if_66
                    else
                      local.get $6
                      local.set $3
                    end ;; $if_65
                  end ;; $block_8
                else
                  local.get $6
                  local.set $3
                end ;; $if_57
                local.get $2
                i32.const 16
                i32.lt_u
                if $if_67
                  local.get $5
                  local.get $2
                  local.get $4
                  i32.add
                  local.tee $0
                  i32.const 3
                  i32.or
                  i32.store offset=4
                  local.get $0
                  local.get $5
                  i32.add
                  i32.const 4
                  i32.add
                  local.tee $0
                  local.get $0
                  i32.load
                  i32.const 1
                  i32.or
                  i32.store
                else
                  block $block_9
                    local.get $5
                    local.get $4
                    i32.const 3
                    i32.or
                    i32.store offset=4
                    local.get $8
                    local.get $2
                    i32.const 1
                    i32.or
                    i32.store offset=4
                    local.get $2
                    local.get $8
                    i32.add
                    local.get $2
                    i32.store
                    local.get $2
                    i32.const 3
                    i32.shr_u
                    local.set $1
                    local.get $2
                    i32.const 256
                    i32.lt_u
                    if $if_68
                      local.get $1
                      i32.const 3
                      i32.shl
                      i32.const 1116
                      i32.add
                      local.set $0
                      i32.const 1076
                      i32.load
                      local.tee $3
                      i32.const 1
                      local.get $1
                      i32.shl
                      local.tee $1
                      i32.and
                      if $if_69
                        i32.const 1092
                        i32.load
                        local.get $0
                        i32.const 8
                        i32.add
                        local.tee $3
                        i32.load
                        local.tee $1
                        i32.gt_u
                        if $if_70
                          call $_abort
                        else
                          local.get $1
                          local.set $13
                          local.get $3
                          local.set $19
                        end ;; $if_70
                      else
                        i32.const 1076
                        local.get $1
                        local.get $3
                        i32.or
                        i32.store
                        local.get $0
                        local.set $13
                        local.get $0
                        i32.const 8
                        i32.add
                        local.set $19
                      end ;; $if_69
                      local.get $19
                      local.get $8
                      i32.store
                      local.get $13
                      local.get $8
                      i32.store offset=12
                      local.get $8
                      local.get $13
                      i32.store offset=8
                      local.get $8
                      local.get $0
                      i32.store offset=12
                      br $block_9
                    end ;; $if_68
                    local.get $2
                    i32.const 8
                    i32.shr_u
                    local.tee $0
                    if $if_71 (result i32)
                      local.get $2
                      i32.const 16777215
                      i32.gt_u
                      if $if_72 (result i32)
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
                        local.tee $1
                        i32.shl
                        local.tee $4
                        i32.const 520192
                        i32.add
                        i32.const 16
                        i32.shr_u
                        i32.const 4
                        i32.and
                        local.set $0
                        i32.const 14
                        local.get $0
                        local.get $1
                        i32.or
                        local.get $4
                        local.get $0
                        i32.shl
                        local.tee $0
                        i32.const 245760
                        i32.add
                        i32.const 16
                        i32.shr_u
                        i32.const 2
                        i32.and
                        local.tee $1
                        i32.or
                        i32.sub
                        local.get $0
                        local.get $1
                        i32.shl
                        i32.const 15
                        i32.shr_u
                        i32.add
                        local.tee $0
                        i32.const 1
                        i32.shl
                        local.get $2
                        local.get $0
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
                    local.tee $1
                    i32.const 2
                    i32.shl
                    i32.const 1380
                    i32.add
                    local.set $0
                    local.get $8
                    local.get $1
                    i32.store offset=28
                    local.get $8
                    i32.const 16
                    i32.add
                    local.tee $4
                    i32.const 0
                    i32.store offset=4
                    local.get $4
                    i32.const 0
                    i32.store
                    i32.const 1
                    local.get $1
                    i32.shl
                    local.tee $4
                    local.get $3
                    i32.and
                    i32.eqz
                    if $if_73
                      i32.const 1080
                      local.get $3
                      local.get $4
                      i32.or
                      i32.store
                      local.get $0
                      local.get $8
                      i32.store
                      local.get $8
                      local.get $0
                      i32.store offset=24
                      local.get $8
                      local.get $8
                      i32.store offset=12
                      local.get $8
                      local.get $8
                      i32.store offset=8
                      br $block_9
                    end ;; $if_73
                    local.get $2
                    local.get $0
                    i32.load
                    local.tee $0
                    i32.load offset=4
                    i32.const -8
                    i32.and
                    i32.eq
                    if $if_74
                      local.get $0
                      local.set $10
                    else
                      block $block_10
                        local.get $2
                        i32.const 0
                        i32.const 25
                        local.get $1
                        i32.const 1
                        i32.shr_u
                        i32.sub
                        local.get $1
                        i32.const 31
                        i32.eq
                        select
                        i32.shl
                        local.set $1
                        loop $loop_4
                          local.get $0
                          i32.const 16
                          i32.add
                          local.get $1
                          i32.const 31
                          i32.shr_u
                          i32.const 2
                          i32.shl
                          i32.add
                          local.tee $4
                          i32.load
                          local.tee $3
                          if $if_75
                            local.get $1
                            i32.const 1
                            i32.shl
                            local.set $1
                            local.get $2
                            local.get $3
                            i32.load offset=4
                            i32.const -8
                            i32.and
                            i32.eq
                            if $if_76
                              local.get $3
                              local.set $10
                              br $block_10
                            else
                              local.get $3
                              local.set $0
                              br $loop_4
                            end ;; $if_76
                            unreachable
                          end ;; $if_75
                        end ;; $loop_4
                        i32.const 1092
                        i32.load
                        local.get $4
                        i32.gt_u
                        if $if_77
                          call $_abort
                        else
                          local.get $4
                          local.get $8
                          i32.store
                          local.get $8
                          local.get $0
                          i32.store offset=24
                          local.get $8
                          local.get $8
                          i32.store offset=12
                          local.get $8
                          local.get $8
                          i32.store offset=8
                          br $block_9
                        end ;; $if_77
                      end ;; $block_10
                    end ;; $if_74
                    i32.const 1092
                    i32.load
                    local.tee $0
                    local.get $10
                    i32.le_u
                    local.get $0
                    local.get $10
                    i32.const 8
                    i32.add
                    local.tee $3
                    i32.load
                    local.tee $0
                    i32.le_u
                    i32.and
                    if $if_78
                      local.get $0
                      local.get $8
                      i32.store offset=12
                      local.get $3
                      local.get $8
                      i32.store
                      local.get $8
                      local.get $0
                      i32.store offset=8
                      local.get $8
                      local.get $10
                      i32.store offset=12
                      local.get $8
                      i32.const 0
                      i32.store offset=24
                    else
                      call $_abort
                    end ;; $if_78
                  end ;; $block_9
                end ;; $if_67
                local.get $14
                global.set $16
                local.get $5
                i32.const 8
                i32.add
                return
              else
                local.get $4
              end ;; $if_47
            else
              local.get $4
            end ;; $if_46
          else
            local.get $4
          end ;; $if_37
        end ;; $block_3
      end ;; $if_36
    end ;; $if
    local.set $3
    block $block_11
      i32.const 1084
      i32.load
      local.tee $1
      local.get $3
      i32.ge_u
      if $if_79
        i32.const 1096
        i32.load
        local.set $0
        local.get $1
        local.get $3
        i32.sub
        local.tee $2
        i32.const 15
        i32.gt_u
        if $if_80
          i32.const 1096
          local.get $0
          local.get $3
          i32.add
          local.tee $4
          i32.store
          i32.const 1084
          local.get $2
          i32.store
          local.get $4
          local.get $2
          i32.const 1
          i32.or
          i32.store offset=4
          local.get $0
          local.get $1
          i32.add
          local.get $2
          i32.store
          local.get $0
          local.get $3
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
          local.get $0
          local.get $1
          i32.const 3
          i32.or
          i32.store offset=4
          local.get $0
          local.get $1
          i32.add
          i32.const 4
          i32.add
          local.tee $3
          local.get $3
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
        local.tee $1
        local.get $3
        i32.gt_u
        if $if_81
          i32.const 1088
          local.get $1
          local.get $3
          i32.sub
          local.tee $1
          i32.store
          br $block_12
        end ;; $if_81
        local.get $3
        i32.const 47
        i32.add
        local.tee $6
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
          local.get $14
          i32.const -16
          i32.and
          i32.const 1431655768
          i32.xor
          i32.store
          i32.const 4096
        end ;; $if_82
        local.tee $0
        i32.add
        local.tee $5
        i32.const 0
        local.get $0
        i32.sub
        local.tee $7
        i32.and
        local.tee $4
        local.get $3
        i32.le_u
        if $if_83
          local.get $14
          global.set $16
          i32.const 0
          return
        end ;; $if_83
        i32.const 1516
        i32.load
        local.tee $0
        if $if_84
          local.get $4
          i32.const 1508
          i32.load
          local.tee $2
          i32.add
          local.tee $10
          local.get $2
          i32.le_u
          local.get $10
          local.get $0
          i32.gt_u
          i32.or
          if $if_85
            local.get $14
            global.set $16
            i32.const 0
            return
          end ;; $if_85
        end ;; $if_84
        local.get $3
        i32.const 48
        i32.add
        local.set $10
        block $block_13
          block $block_14
            i32.const 1520
            i32.load
            i32.const 4
            i32.and
            if $if_86
              i32.const 0
              local.set $1
            else
              block $block_15
                block $block_16
                  block $block_17
                    i32.const 1100
                    i32.load
                    local.tee $0
                    i32.eqz
                    br_if $block_17
                    i32.const 1524
                    local.set $2
                    loop $loop_5
                      block $block_18
                        local.get $2
                        i32.load
                        local.tee $13
                        local.get $0
                        i32.le_u
                        if $if_87
                          local.get $13
                          local.get $2
                          i32.load offset=4
                          i32.add
                          local.get $0
                          i32.gt_u
                          br_if $block_18
                        end ;; $if_87
                        local.get $2
                        i32.load offset=8
                        local.tee $2
                        br_if $loop_5
                        br $block_17
                      end ;; $block_18
                    end ;; $loop_5
                    local.get $7
                    local.get $5
                    local.get $1
                    i32.sub
                    i32.and
                    local.tee $1
                    i32.const 2147483647
                    i32.lt_u
                    if $if_88
                      local.get $2
                      i32.const 4
                      i32.add
                      local.set $5
                      local.get $1
                      call $_sbrk
                      local.tee $0
                      local.get $2
                      i32.load
                      local.get $5
                      i32.load
                      i32.add
                      i32.ne
                      br_if $block_16
                      local.get $0
                      i32.const -1
                      i32.ne
                      br_if $block_14
                    else
                      i32.const 0
                      local.set $1
                    end ;; $if_88
                    br $block_15
                  end ;; $block_17
                  i32.const 0
                  call $_sbrk
                  local.tee $0
                  i32.const -1
                  i32.eq
                  if $if_89 (result i32)
                    i32.const 0
                  else
                    i32.const 1508
                    i32.load
                    local.tee $5
                    local.get $4
                    local.get $0
                    i32.const 1552
                    i32.load
                    local.tee $1
                    i32.const -1
                    i32.add
                    local.tee $2
                    i32.add
                    i32.const 0
                    local.get $1
                    i32.sub
                    i32.and
                    local.get $0
                    i32.sub
                    i32.const 0
                    local.get $0
                    local.get $2
                    i32.and
                    select
                    i32.add
                    local.tee $1
                    i32.add
                    local.set $2
                    local.get $1
                    i32.const 2147483647
                    i32.lt_u
                    local.get $1
                    local.get $3
                    i32.gt_u
                    i32.and
                    if $if_90 (result i32)
                      i32.const 1516
                      i32.load
                      local.tee $7
                      if $if_91
                        local.get $2
                        local.get $5
                        i32.le_u
                        local.get $2
                        local.get $7
                        i32.gt_u
                        i32.or
                        if $if_92
                          i32.const 0
                          local.set $1
                          br $block_15
                        end ;; $if_92
                      end ;; $if_91
                      local.get $0
                      local.get $1
                      call $_sbrk
                      local.tee $2
                      i32.eq
                      br_if $block_14
                      local.get $2
                      local.set $0
                      br $block_16
                    else
                      i32.const 0
                    end ;; $if_90
                  end ;; $if_89
                  local.set $1
                  br $block_15
                end ;; $block_16
                local.get $0
                i32.const -1
                i32.ne
                local.get $1
                i32.const 2147483647
                i32.lt_u
                i32.and
                local.get $10
                local.get $1
                i32.gt_u
                i32.and
                i32.eqz
                if $if_93
                  local.get $0
                  i32.const -1
                  i32.eq
                  if $if_94
                    i32.const 0
                    local.set $1
                    br $block_15
                  else
                    br $block_14
                  end ;; $if_94
                  unreachable
                end ;; $if_93
                i32.const 1556
                i32.load
                local.tee $2
                local.get $6
                local.get $1
                i32.sub
                i32.add
                i32.const 0
                local.get $2
                i32.sub
                i32.and
                local.tee $2
                i32.const 2147483647
                i32.ge_u
                br_if $block_14
                i32.const 0
                local.get $1
                i32.sub
                local.set $6
                local.get $2
                call $_sbrk
                i32.const -1
                i32.eq
                if $if_95 (result i32)
                  local.get $6
                  call $_sbrk
                  drop
                  i32.const 0
                else
                  local.get $1
                  local.get $2
                  i32.add
                  local.set $1
                  br $block_14
                end ;; $if_95
                local.set $1
              end ;; $block_15
              i32.const 1520
              i32.const 1520
              i32.load
              i32.const 4
              i32.or
              i32.store
            end ;; $if_86
            local.get $4
            i32.const 2147483647
            i32.lt_u
            if $if_96
              local.get $4
              call $_sbrk
              local.set $0
              i32.const 0
              call $_sbrk
              local.tee $2
              local.get $0
              i32.sub
              local.tee $6
              local.get $3
              i32.const 40
              i32.add
              i32.gt_u
              local.set $4
              local.get $6
              local.get $1
              local.get $4
              select
              local.set $1
              local.get $4
              i32.const 1
              i32.xor
              local.get $0
              i32.const -1
              i32.eq
              i32.or
              local.get $0
              i32.const -1
              i32.ne
              local.get $2
              i32.const -1
              i32.ne
              i32.and
              local.get $0
              local.get $2
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
          local.get $1
          i32.const 1508
          i32.load
          i32.add
          local.tee $2
          i32.store
          local.get $2
          i32.const 1512
          i32.load
          i32.gt_u
          if $if_97
            i32.const 1512
            local.get $2
            i32.store
          end ;; $if_97
          i32.const 1100
          i32.load
          local.tee $6
          if $if_98
            block $block_19
              i32.const 1524
              local.set $2
              block $block_20
                block $block_21
                  loop $loop_6
                    local.get $0
                    local.get $2
                    i32.load
                    local.tee $4
                    local.get $2
                    i32.load offset=4
                    local.tee $5
                    i32.add
                    i32.eq
                    br_if $block_21
                    local.get $2
                    i32.load offset=8
                    local.tee $2
                    br_if $loop_6
                  end ;; $loop_6
                  br $block_20
                end ;; $block_21
                local.get $2
                i32.const 4
                i32.add
                local.set $7
                local.get $2
                i32.load offset=12
                i32.const 8
                i32.and
                i32.eqz
                if $if_99
                  local.get $4
                  local.get $6
                  i32.le_u
                  local.get $0
                  local.get $6
                  i32.gt_u
                  i32.and
                  if $if_100
                    local.get $7
                    local.get $1
                    local.get $5
                    i32.add
                    i32.store
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
                    local.tee $2
                    i32.add
                    local.set $0
                    local.get $1
                    i32.const 1088
                    i32.load
                    i32.add
                    local.tee $4
                    local.get $2
                    i32.sub
                    local.set $1
                    i32.const 1100
                    local.get $0
                    i32.store
                    i32.const 1088
                    local.get $1
                    i32.store
                    local.get $0
                    local.get $1
                    i32.const 1
                    i32.or
                    i32.store offset=4
                    local.get $4
                    local.get $6
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
              local.get $0
              i32.const 1092
              i32.load
              local.tee $2
              i32.lt_u
              if $if_101
                i32.const 1092
                local.get $0
                i32.store
                local.get $0
                local.set $2
              end ;; $if_101
              local.get $0
              local.get $1
              i32.add
              local.set $5
              i32.const 1524
              local.set $4
              block $block_22
                block $block_23
                  loop $loop_7
                    local.get $5
                    local.get $4
                    i32.load
                    i32.eq
                    br_if $block_23
                    local.get $4
                    i32.load offset=8
                    local.tee $4
                    br_if $loop_7
                  end ;; $loop_7
                  br $block_22
                end ;; $block_23
                local.get $4
                i32.load offset=12
                i32.const 8
                i32.and
                i32.eqz
                if $if_102
                  local.get $4
                  local.get $0
                  i32.store
                  local.get $4
                  i32.const 4
                  i32.add
                  local.tee $4
                  local.get $1
                  local.get $4
                  i32.load
                  i32.add
                  i32.store
                  local.get $3
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
                  local.tee $8
                  i32.add
                  local.set $7
                  local.get $5
                  i32.const 0
                  local.get $5
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
                  local.tee $1
                  local.get $8
                  i32.sub
                  local.get $3
                  i32.sub
                  local.set $4
                  local.get $8
                  local.get $3
                  i32.const 3
                  i32.or
                  i32.store offset=4
                  local.get $1
                  local.get $6
                  i32.eq
                  if $if_103
                    i32.const 1088
                    local.get $4
                    i32.const 1088
                    i32.load
                    i32.add
                    local.tee $0
                    i32.store
                    i32.const 1100
                    local.get $7
                    i32.store
                    local.get $7
                    local.get $0
                    i32.const 1
                    i32.or
                    i32.store offset=4
                  else
                    block $block_24
                      local.get $1
                      i32.const 1096
                      i32.load
                      i32.eq
                      if $if_104
                        i32.const 1084
                        local.get $4
                        i32.const 1084
                        i32.load
                        i32.add
                        local.tee $0
                        i32.store
                        i32.const 1096
                        local.get $7
                        i32.store
                        local.get $7
                        local.get $0
                        i32.const 1
                        i32.or
                        i32.store offset=4
                        local.get $0
                        local.get $7
                        i32.add
                        local.get $0
                        i32.store
                        br $block_24
                      end ;; $if_104
                      local.get $1
                      i32.load offset=4
                      local.tee $0
                      i32.const 3
                      i32.and
                      i32.const 1
                      i32.eq
                      if $if_105 (result i32)
                        local.get $0
                        i32.const -8
                        i32.and
                        local.set $13
                        local.get $0
                        i32.const 3
                        i32.shr_u
                        local.set $5
                        block $block_25
                          local.get $0
                          i32.const 256
                          i32.lt_u
                          if $if_106
                            local.get $1
                            i32.load offset=12
                            local.set $3
                            local.get $1
                            i32.load offset=8
                            local.tee $6
                            local.get $5
                            i32.const 3
                            i32.shl
                            i32.const 1116
                            i32.add
                            local.tee $0
                            i32.ne
                            if $if_107
                              block $block_26
                                local.get $2
                                local.get $6
                                i32.gt_u
                                if $if_108
                                  call $_abort
                                end ;; $if_108
                                local.get $1
                                local.get $6
                                i32.load offset=12
                                i32.eq
                                br_if $block_26
                                call $_abort
                              end ;; $block_26
                            end ;; $if_107
                            local.get $3
                            local.get $6
                            i32.eq
                            if $if_109
                              i32.const 1076
                              i32.const 1076
                              i32.load
                              i32.const 1
                              local.get $5
                              i32.shl
                              i32.const -1
                              i32.xor
                              i32.and
                              i32.store
                              br $block_25
                            end ;; $if_109
                            local.get $0
                            local.get $3
                            i32.eq
                            if $if_110
                              local.get $3
                              i32.const 8
                              i32.add
                              local.set $20
                            else
                              block $block_27
                                local.get $2
                                local.get $3
                                i32.gt_u
                                if $if_111
                                  call $_abort
                                end ;; $if_111
                                local.get $1
                                local.get $3
                                i32.const 8
                                i32.add
                                local.tee $0
                                i32.load
                                i32.eq
                                if $if_112
                                  local.get $0
                                  local.set $20
                                  br $block_27
                                end ;; $if_112
                                call $_abort
                              end ;; $block_27
                            end ;; $if_110
                            local.get $6
                            local.get $3
                            i32.store offset=12
                            local.get $20
                            local.get $6
                            i32.store
                          else
                            local.get $1
                            i32.load offset=24
                            local.set $10
                            local.get $1
                            local.get $1
                            i32.load offset=12
                            local.tee $0
                            i32.eq
                            if $if_113
                              block $block_28
                                local.get $1
                                i32.const 16
                                i32.add
                                local.tee $3
                                i32.const 4
                                i32.add
                                local.tee $6
                                i32.load
                                local.tee $0
                                if $if_114
                                  local.get $6
                                  local.set $3
                                else
                                  local.get $3
                                  i32.load
                                  local.tee $0
                                  i32.eqz
                                  br_if $block_28
                                end ;; $if_114
                                loop $loop_8
                                  block $block_29
                                    local.get $0
                                    i32.const 20
                                    i32.add
                                    local.tee $6
                                    i32.load
                                    local.tee $5
                                    i32.eqz
                                    if $if_115
                                      local.get $0
                                      i32.const 16
                                      i32.add
                                      local.tee $6
                                      i32.load
                                      local.tee $5
                                      i32.eqz
                                      br_if $block_29
                                    end ;; $if_115
                                    local.get $6
                                    local.set $3
                                    local.get $5
                                    local.set $0
                                    br $loop_8
                                  end ;; $block_29
                                end ;; $loop_8
                                local.get $2
                                local.get $3
                                i32.gt_u
                                if $if_116
                                  call $_abort
                                else
                                  local.get $3
                                  i32.const 0
                                  i32.store
                                  local.get $0
                                  local.set $9
                                end ;; $if_116
                              end ;; $block_28
                            else
                              local.get $2
                              local.get $1
                              i32.load offset=8
                              local.tee $3
                              i32.gt_u
                              if $if_117
                                call $_abort
                              end ;; $if_117
                              local.get $3
                              i32.const 12
                              i32.add
                              local.tee $2
                              i32.load
                              local.get $1
                              i32.ne
                              if $if_118
                                call $_abort
                              end ;; $if_118
                              local.get $1
                              local.get $0
                              i32.const 8
                              i32.add
                              local.tee $6
                              i32.load
                              i32.eq
                              if $if_119
                                local.get $2
                                local.get $0
                                i32.store
                                local.get $6
                                local.get $3
                                i32.store
                                local.get $0
                                local.set $9
                              else
                                call $_abort
                              end ;; $if_119
                            end ;; $if_113
                            local.get $10
                            i32.eqz
                            br_if $block_25
                            local.get $1
                            local.get $1
                            i32.load offset=28
                            local.tee $0
                            i32.const 2
                            i32.shl
                            i32.const 1380
                            i32.add
                            local.tee $3
                            i32.load
                            i32.eq
                            if $if_120
                              block $block_30
                                local.get $3
                                local.get $9
                                i32.store
                                local.get $9
                                br_if $block_30
                                i32.const 1080
                                i32.const 1080
                                i32.load
                                i32.const 1
                                local.get $0
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
                              local.get $10
                              i32.gt_u
                              if $if_121
                                call $_abort
                              else
                                local.get $10
                                i32.const 16
                                i32.add
                                local.tee $0
                                local.get $10
                                i32.const 20
                                i32.add
                                local.get $1
                                local.get $0
                                i32.load
                                i32.eq
                                select
                                local.get $9
                                i32.store
                                local.get $9
                                i32.eqz
                                br_if $block_25
                              end ;; $if_121
                            end ;; $if_120
                            i32.const 1092
                            i32.load
                            local.tee $3
                            local.get $9
                            i32.gt_u
                            if $if_122
                              call $_abort
                            end ;; $if_122
                            local.get $9
                            local.get $10
                            i32.store offset=24
                            local.get $1
                            i32.const 16
                            i32.add
                            local.tee $2
                            i32.load
                            local.tee $0
                            if $if_123
                              local.get $3
                              local.get $0
                              i32.gt_u
                              if $if_124
                                call $_abort
                              else
                                local.get $9
                                local.get $0
                                i32.store offset=16
                                local.get $0
                                local.get $9
                                i32.store offset=24
                              end ;; $if_124
                            end ;; $if_123
                            local.get $2
                            i32.load offset=4
                            local.tee $0
                            i32.eqz
                            br_if $block_25
                            i32.const 1092
                            i32.load
                            local.get $0
                            i32.gt_u
                            if $if_125
                              call $_abort
                            else
                              local.get $9
                              local.get $0
                              i32.store offset=20
                              local.get $0
                              local.get $9
                              i32.store offset=24
                            end ;; $if_125
                          end ;; $if_106
                        end ;; $block_25
                        local.get $1
                        local.get $13
                        i32.add
                        local.set $1
                        local.get $4
                        local.get $13
                        i32.add
                      else
                        local.get $4
                      end ;; $if_105
                      local.set $2
                      local.get $1
                      i32.const 4
                      i32.add
                      local.tee $0
                      local.get $0
                      i32.load
                      i32.const -2
                      i32.and
                      i32.store
                      local.get $7
                      local.get $2
                      i32.const 1
                      i32.or
                      i32.store offset=4
                      local.get $2
                      local.get $7
                      i32.add
                      local.get $2
                      i32.store
                      local.get $2
                      i32.const 3
                      i32.shr_u
                      local.set $3
                      local.get $2
                      i32.const 256
                      i32.lt_u
                      if $if_126
                        local.get $3
                        i32.const 3
                        i32.shl
                        i32.const 1116
                        i32.add
                        local.set $0
                        i32.const 1076
                        i32.load
                        local.tee $1
                        i32.const 1
                        local.get $3
                        i32.shl
                        local.tee $3
                        i32.and
                        if $if_127
                          block $block_31
                            i32.const 1092
                            i32.load
                            local.get $0
                            i32.const 8
                            i32.add
                            local.tee $3
                            i32.load
                            local.tee $1
                            i32.le_u
                            if $if_128
                              local.get $1
                              local.set $15
                              local.get $3
                              local.set $21
                              br $block_31
                            end ;; $if_128
                            call $_abort
                          end ;; $block_31
                        else
                          i32.const 1076
                          local.get $1
                          local.get $3
                          i32.or
                          i32.store
                          local.get $0
                          local.set $15
                          local.get $0
                          i32.const 8
                          i32.add
                          local.set $21
                        end ;; $if_127
                        local.get $21
                        local.get $7
                        i32.store
                        local.get $15
                        local.get $7
                        i32.store offset=12
                        local.get $7
                        local.get $15
                        i32.store offset=8
                        local.get $7
                        local.get $0
                        i32.store offset=12
                        br $block_24
                      end ;; $if_126
                      local.get $2
                      i32.const 8
                      i32.shr_u
                      local.tee $0
                      if $if_129 (result i32)
                        local.get $2
                        i32.const 16777215
                        i32.gt_u
                        if $if_130 (result i32)
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
                          local.tee $3
                          i32.shl
                          local.tee $1
                          i32.const 520192
                          i32.add
                          i32.const 16
                          i32.shr_u
                          i32.const 4
                          i32.and
                          local.set $0
                          i32.const 14
                          local.get $0
                          local.get $3
                          i32.or
                          local.get $1
                          local.get $0
                          i32.shl
                          local.tee $0
                          i32.const 245760
                          i32.add
                          i32.const 16
                          i32.shr_u
                          i32.const 2
                          i32.and
                          local.tee $3
                          i32.or
                          i32.sub
                          local.get $0
                          local.get $3
                          i32.shl
                          i32.const 15
                          i32.shr_u
                          i32.add
                          local.tee $0
                          i32.const 1
                          i32.shl
                          local.get $2
                          local.get $0
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
                      local.tee $3
                      i32.const 2
                      i32.shl
                      i32.const 1380
                      i32.add
                      local.set $0
                      local.get $7
                      local.get $3
                      i32.store offset=28
                      local.get $7
                      i32.const 16
                      i32.add
                      local.tee $1
                      i32.const 0
                      i32.store offset=4
                      local.get $1
                      i32.const 0
                      i32.store
                      i32.const 1080
                      i32.load
                      local.tee $1
                      i32.const 1
                      local.get $3
                      i32.shl
                      local.tee $4
                      i32.and
                      i32.eqz
                      if $if_131
                        i32.const 1080
                        local.get $1
                        local.get $4
                        i32.or
                        i32.store
                        local.get $0
                        local.get $7
                        i32.store
                        local.get $7
                        local.get $0
                        i32.store offset=24
                        local.get $7
                        local.get $7
                        i32.store offset=12
                        local.get $7
                        local.get $7
                        i32.store offset=8
                        br $block_24
                      end ;; $if_131
                      local.get $2
                      local.get $0
                      i32.load
                      local.tee $0
                      i32.load offset=4
                      i32.const -8
                      i32.and
                      i32.eq
                      if $if_132
                        local.get $0
                        local.set $11
                      else
                        block $block_32
                          local.get $2
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
                          local.set $1
                          loop $loop_9
                            local.get $0
                            i32.const 16
                            i32.add
                            local.get $1
                            i32.const 31
                            i32.shr_u
                            i32.const 2
                            i32.shl
                            i32.add
                            local.tee $4
                            i32.load
                            local.tee $3
                            if $if_133
                              local.get $1
                              i32.const 1
                              i32.shl
                              local.set $1
                              local.get $2
                              local.get $3
                              i32.load offset=4
                              i32.const -8
                              i32.and
                              i32.eq
                              if $if_134
                                local.get $3
                                local.set $11
                                br $block_32
                              else
                                local.get $3
                                local.set $0
                                br $loop_9
                              end ;; $if_134
                              unreachable
                            end ;; $if_133
                          end ;; $loop_9
                          i32.const 1092
                          i32.load
                          local.get $4
                          i32.gt_u
                          if $if_135
                            call $_abort
                          else
                            local.get $4
                            local.get $7
                            i32.store
                            local.get $7
                            local.get $0
                            i32.store offset=24
                            local.get $7
                            local.get $7
                            i32.store offset=12
                            local.get $7
                            local.get $7
                            i32.store offset=8
                            br $block_24
                          end ;; $if_135
                        end ;; $block_32
                      end ;; $if_132
                      i32.const 1092
                      i32.load
                      local.tee $0
                      local.get $11
                      i32.le_u
                      local.get $0
                      local.get $11
                      i32.const 8
                      i32.add
                      local.tee $3
                      i32.load
                      local.tee $0
                      i32.le_u
                      i32.and
                      if $if_136
                        local.get $0
                        local.get $7
                        i32.store offset=12
                        local.get $3
                        local.get $7
                        i32.store
                        local.get $7
                        local.get $0
                        i32.store offset=8
                        local.get $7
                        local.get $11
                        i32.store offset=12
                        local.get $7
                        i32.const 0
                        i32.store offset=24
                      else
                        call $_abort
                      end ;; $if_136
                    end ;; $block_24
                  end ;; $if_103
                  local.get $14
                  global.set $16
                  local.get $8
                  i32.const 8
                  i32.add
                  return
                end ;; $if_102
              end ;; $block_22
              i32.const 1524
              local.set $2
              loop $loop_10
                block $block_33
                  local.get $2
                  i32.load
                  local.tee $4
                  local.get $6
                  i32.le_u
                  if $if_137
                    local.get $4
                    local.get $2
                    i32.load offset=4
                    i32.add
                    local.tee $5
                    local.get $6
                    i32.gt_u
                    br_if $block_33
                  end ;; $if_137
                  local.get $2
                  i32.load offset=8
                  local.set $2
                  br $loop_10
                end ;; $block_33
              end ;; $loop_10
              local.get $5
              i32.const -47
              i32.add
              local.tee $4
              i32.const 8
              i32.add
              local.set $2
              local.get $6
              local.get $4
              i32.const 0
              local.get $2
              i32.sub
              i32.const 7
              i32.and
              i32.const 0
              local.get $2
              i32.const 7
              i32.and
              select
              i32.add
              local.tee $2
              local.get $2
              local.get $6
              i32.const 16
              i32.add
              local.tee $9
              i32.lt_u
              select
              local.tee $2
              i32.const 8
              i32.add
              local.set $4
              i32.const 1100
              local.get $0
              i32.const 0
              local.get $0
              i32.const 8
              i32.add
              local.tee $7
              i32.sub
              i32.const 7
              i32.and
              i32.const 0
              local.get $7
              i32.const 7
              i32.and
              select
              local.tee $7
              i32.add
              local.tee $10
              i32.store
              i32.const 1088
              local.get $1
              i32.const -40
              i32.add
              local.tee $11
              local.get $7
              i32.sub
              local.tee $7
              i32.store
              local.get $10
              local.get $7
              i32.const 1
              i32.or
              i32.store offset=4
              local.get $0
              local.get $11
              i32.add
              i32.const 40
              i32.store offset=4
              i32.const 1104
              i32.const 1564
              i32.load
              i32.store
              local.get $2
              i32.const 4
              i32.add
              local.tee $7
              i32.const 27
              i32.store
              local.get $4
              i32.const 1524
              i64.load align=4
              i64.store align=4
              local.get $4
              i32.const 1532
              i64.load align=4
              i64.store offset=8 align=4
              i32.const 1524
              local.get $0
              i32.store
              i32.const 1528
              local.get $1
              i32.store
              i32.const 1536
              i32.const 0
              i32.store
              i32.const 1532
              local.get $4
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
                local.get $5
                i32.lt_u
                if $if_138
                  local.get $1
                  local.set $0
                  br $loop_11
                end ;; $if_138
              end ;; $loop_11
              local.get $2
              local.get $6
              i32.ne
              if $if_139
                local.get $7
                local.get $7
                i32.load
                i32.const -2
                i32.and
                i32.store
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
                local.set $1
                local.get $4
                i32.const 256
                i32.lt_u
                if $if_140
                  local.get $1
                  i32.const 3
                  i32.shl
                  i32.const 1116
                  i32.add
                  local.set $0
                  i32.const 1076
                  i32.load
                  local.tee $2
                  i32.const 1
                  local.get $1
                  i32.shl
                  local.tee $1
                  i32.and
                  if $if_141
                    i32.const 1092
                    i32.load
                    local.get $0
                    i32.const 8
                    i32.add
                    local.tee $1
                    i32.load
                    local.tee $2
                    i32.gt_u
                    if $if_142
                      call $_abort
                    else
                      local.get $2
                      local.set $17
                      local.get $1
                      local.set $22
                    end ;; $if_142
                  else
                    i32.const 1076
                    local.get $1
                    local.get $2
                    i32.or
                    i32.store
                    local.get $0
                    local.set $17
                    local.get $0
                    i32.const 8
                    i32.add
                    local.set $22
                  end ;; $if_141
                  local.get $22
                  local.get $6
                  i32.store
                  local.get $17
                  local.get $6
                  i32.store offset=12
                  local.get $6
                  local.get $17
                  i32.store offset=8
                  local.get $6
                  local.get $0
                  i32.store offset=12
                  br $block_19
                end ;; $if_140
                local.get $4
                i32.const 8
                i32.shr_u
                local.tee $0
                if $if_143 (result i32)
                  local.get $4
                  i32.const 16777215
                  i32.gt_u
                  if $if_144 (result i32)
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
                    local.tee $1
                    i32.shl
                    local.tee $2
                    i32.const 520192
                    i32.add
                    i32.const 16
                    i32.shr_u
                    i32.const 4
                    i32.and
                    local.set $0
                    i32.const 14
                    local.get $2
                    local.get $0
                    i32.shl
                    local.tee $2
                    i32.const 245760
                    i32.add
                    i32.const 16
                    i32.shr_u
                    i32.const 2
                    i32.and
                    local.tee $5
                    local.get $0
                    local.get $1
                    i32.or
                    i32.or
                    i32.sub
                    local.get $2
                    local.get $5
                    i32.shl
                    i32.const 15
                    i32.shr_u
                    i32.add
                    local.tee $0
                    i32.const 1
                    i32.shl
                    local.get $4
                    local.get $0
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
                local.tee $1
                i32.const 2
                i32.shl
                i32.const 1380
                i32.add
                local.set $0
                local.get $6
                local.get $1
                i32.store offset=28
                local.get $6
                i32.const 0
                i32.store offset=20
                local.get $9
                i32.const 0
                i32.store
                i32.const 1080
                i32.load
                local.tee $2
                i32.const 1
                local.get $1
                i32.shl
                local.tee $5
                i32.and
                i32.eqz
                if $if_145
                  i32.const 1080
                  local.get $2
                  local.get $5
                  i32.or
                  i32.store
                  local.get $0
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
                  br $block_19
                end ;; $if_145
                local.get $0
                i32.load
                local.tee $0
                i32.load offset=4
                i32.const -8
                i32.and
                local.get $4
                i32.eq
                if $if_146
                  local.get $0
                  local.set $8
                else
                  block $block_34
                    local.get $4
                    i32.const 0
                    i32.const 25
                    local.get $1
                    i32.const 1
                    i32.shr_u
                    i32.sub
                    local.get $1
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
                      local.tee $5
                      i32.load
                      local.tee $1
                      if $if_147
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
                        if $if_148
                          local.get $1
                          local.set $8
                          br $block_34
                        else
                          local.get $1
                          local.set $0
                          br $loop_12
                        end ;; $if_148
                        unreachable
                      end ;; $if_147
                    end ;; $loop_12
                    i32.const 1092
                    i32.load
                    local.get $5
                    i32.gt_u
                    if $if_149
                      call $_abort
                    else
                      local.get $5
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
                      br $block_19
                    end ;; $if_149
                  end ;; $block_34
                end ;; $if_146
                i32.const 1092
                i32.load
                local.tee $0
                local.get $8
                i32.le_u
                local.get $0
                local.get $8
                i32.const 8
                i32.add
                local.tee $1
                i32.load
                local.tee $0
                i32.le_u
                i32.and
                if $if_150
                  local.get $0
                  local.get $6
                  i32.store offset=12
                  local.get $1
                  local.get $6
                  i32.store
                  local.get $6
                  local.get $0
                  i32.store offset=8
                  local.get $6
                  local.get $8
                  i32.store offset=12
                  local.get $6
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
            local.tee $2
            i32.eqz
            local.get $0
            local.get $2
            i32.lt_u
            i32.or
            if $if_151
              i32.const 1092
              local.get $0
              i32.store
            end ;; $if_151
            i32.const 1524
            local.get $0
            i32.store
            i32.const 1528
            local.get $1
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
            local.get $0
            i32.const 0
            local.get $0
            i32.const 8
            i32.add
            local.tee $2
            i32.sub
            i32.const 7
            i32.and
            i32.const 0
            local.get $2
            i32.const 7
            i32.and
            select
            local.tee $2
            i32.add
            local.tee $4
            i32.store
            i32.const 1088
            local.get $1
            i32.const -40
            i32.add
            local.tee $1
            local.get $2
            i32.sub
            local.tee $2
            i32.store
            local.get $4
            local.get $2
            i32.const 1
            i32.or
            i32.store offset=4
            local.get $0
            local.get $1
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
          local.tee $0
          local.get $3
          i32.gt_u
          if $if_152
            i32.const 1088
            local.get $0
            local.get $3
            i32.sub
            local.tee $1
            i32.store
            br $block_12
          end ;; $if_152
        end ;; $block_13
        call $___errno_location
        i32.const 12
        i32.store
        local.get $14
        global.set $16
        i32.const 0
        return
      end ;; $block_12
      i32.const 1100
      local.get $3
      i32.const 1100
      i32.load
      local.tee $0
      i32.add
      local.tee $2
      i32.store
      local.get $2
      local.get $1
      i32.const 1
      i32.or
      i32.store offset=4
      local.get $0
      local.get $3
      i32.const 3
      i32.or
      i32.store offset=4
    end ;; $block_11
    local.get $14
    global.set $16
    local.get $0
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
    local.get $0
    i32.eqz
    if $if
      return
    end ;; $if
    local.get $0
    i32.const -8
    i32.add
    local.tee $4
    i32.const 1092
    i32.load
    local.tee $12
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
    local.tee $11
    i32.const 1
    i32.eq
    if $if_1
      call $_abort
    end ;; $if_1
    local.get $4
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
      local.get $2
      local.set $1
      local.get $4
      local.tee $3
      local.set $5
    else
      block $block
        local.get $4
        i32.load
        local.set $9
        local.get $11
        i32.eqz
        if $if_3
          return
        end ;; $if_3
        local.get $4
        local.get $9
        i32.sub
        local.tee $0
        local.get $12
        i32.lt_u
        if $if_4
          call $_abort
        end ;; $if_4
        local.get $2
        local.get $9
        i32.add
        local.set $4
        local.get $0
        i32.const 1096
        i32.load
        i32.eq
        if $if_5
          local.get $7
          i32.const 4
          i32.add
          local.tee $3
          i32.load
          local.tee $1
          i32.const 3
          i32.and
          i32.const 3
          i32.ne
          if $if_6
            local.get $4
            local.set $1
            local.get $0
            local.tee $3
            local.set $5
            br $block
          end ;; $if_6
          i32.const 1084
          local.get $4
          i32.store
          local.get $3
          local.get $1
          i32.const -2
          i32.and
          i32.store
          local.get $0
          local.get $4
          i32.const 1
          i32.or
          i32.store offset=4
          local.get $0
          local.get $4
          i32.add
          local.get $4
          i32.store
          return
        end ;; $if_5
        local.get $9
        i32.const 3
        i32.shr_u
        local.set $2
        local.get $9
        i32.const 256
        i32.lt_u
        if $if_7
          local.get $0
          i32.load offset=12
          local.set $1
          local.get $0
          i32.load offset=8
          local.tee $5
          local.get $2
          i32.const 3
          i32.shl
          i32.const 1116
          i32.add
          local.tee $3
          i32.ne
          if $if_8
            local.get $12
            local.get $5
            i32.gt_u
            if $if_9
              call $_abort
            end ;; $if_9
            local.get $5
            i32.load offset=12
            local.get $0
            i32.ne
            if $if_10
              call $_abort
            end ;; $if_10
          end ;; $if_8
          local.get $1
          local.get $5
          i32.eq
          if $if_11
            i32.const 1076
            i32.const 1076
            i32.load
            i32.const 1
            local.get $2
            i32.shl
            i32.const -1
            i32.xor
            i32.and
            i32.store
            local.get $4
            local.set $1
            local.get $0
            local.tee $3
            local.set $5
            br $block
          end ;; $if_11
          local.get $1
          local.get $3
          i32.eq
          if $if_12
            local.get $1
            i32.const 8
            i32.add
            local.set $6
          else
            local.get $12
            local.get $1
            i32.gt_u
            if $if_13
              call $_abort
            end ;; $if_13
            local.get $0
            local.get $1
            i32.const 8
            i32.add
            local.tee $3
            i32.load
            i32.eq
            if $if_14
              local.get $3
              local.set $6
            else
              call $_abort
            end ;; $if_14
          end ;; $if_12
          local.get $5
          local.get $1
          i32.store offset=12
          local.get $6
          local.get $5
          i32.store
          local.get $4
          local.set $1
          local.get $0
          local.tee $3
          local.set $5
          br $block
        end ;; $if_7
        local.get $0
        i32.load offset=24
        local.set $13
        local.get $0
        local.get $0
        i32.load offset=12
        local.tee $2
        i32.eq
        if $if_15
          block $block_0
            local.get $0
            i32.const 16
            i32.add
            local.tee $6
            i32.const 4
            i32.add
            local.tee $9
            i32.load
            local.tee $2
            if $if_16
              local.get $9
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
                local.tee $9
                i32.load
                local.tee $11
                i32.eqz
                if $if_17
                  local.get $2
                  i32.const 16
                  i32.add
                  local.tee $9
                  i32.load
                  local.tee $11
                  i32.eqz
                  br_if $block_1
                end ;; $if_17
                local.get $9
                local.set $6
                local.get $11
                local.set $2
                br $loop
              end ;; $block_1
            end ;; $loop
            local.get $12
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
          local.get $12
          local.get $0
          i32.load offset=8
          local.tee $6
          i32.gt_u
          if $if_19
            call $_abort
          end ;; $if_19
          local.get $6
          i32.const 12
          i32.add
          local.tee $9
          i32.load
          local.get $0
          i32.ne
          if $if_20
            call $_abort
          end ;; $if_20
          local.get $0
          local.get $2
          i32.const 8
          i32.add
          local.tee $11
          i32.load
          i32.eq
          if $if_21
            local.get $9
            local.get $2
            i32.store
            local.get $11
            local.get $6
            i32.store
            local.get $2
            local.set $8
          else
            call $_abort
          end ;; $if_21
        end ;; $if_15
        local.get $13
        if $if_22
          local.get $0
          local.get $0
          i32.load offset=28
          local.tee $2
          i32.const 2
          i32.shl
          i32.const 1380
          i32.add
          local.tee $6
          i32.load
          i32.eq
          if $if_23
            local.get $6
            local.get $8
            i32.store
            local.get $8
            i32.eqz
            if $if_24
              i32.const 1080
              i32.const 1080
              i32.load
              i32.const 1
              local.get $2
              i32.shl
              i32.const -1
              i32.xor
              i32.and
              i32.store
              local.get $4
              local.set $1
              local.get $0
              local.tee $3
              local.set $5
              br $block
            end ;; $if_24
          else
            i32.const 1092
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
              local.get $0
              local.get $2
              i32.load
              i32.eq
              select
              local.get $8
              i32.store
              local.get $8
              i32.eqz
              if $if_26
                local.get $4
                local.set $1
                local.get $0
                local.tee $3
                local.set $5
                br $block
              end ;; $if_26
            end ;; $if_25
          end ;; $if_23
          i32.const 1092
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
          i32.const 16
          i32.add
          local.tee $9
          i32.load
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
          local.get $9
          i32.load offset=4
          local.tee $2
          if $if_30
            i32.const 1092
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
              local.get $4
              local.set $1
              local.get $0
              local.tee $3
              local.set $5
            end ;; $if_31
          else
            local.get $4
            local.set $1
            local.get $0
            local.tee $3
            local.set $5
          end ;; $if_30
        else
          local.get $4
          local.set $1
          local.get $0
          local.tee $3
          local.set $5
        end ;; $if_22
      end ;; $block
    end ;; $if_2
    local.get $5
    local.get $7
    i32.ge_u
    if $if_32
      call $_abort
    end ;; $if_32
    local.get $7
    i32.const 4
    i32.add
    local.tee $4
    i32.load
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
    if $if_34 (result i32)
      local.get $4
      local.get $0
      i32.const -2
      i32.and
      i32.store
      local.get $3
      local.get $1
      i32.const 1
      i32.or
      i32.store offset=4
      local.get $1
      local.get $5
      i32.add
      local.get $1
      i32.store
      local.get $1
    else
      local.get $7
      i32.const 1100
      i32.load
      i32.eq
      if $if_35
        i32.const 1088
        local.get $1
        i32.const 1088
        i32.load
        i32.add
        local.tee $0
        i32.store
        i32.const 1100
        local.get $3
        i32.store
        local.get $3
        local.get $0
        i32.const 1
        i32.or
        i32.store offset=4
        i32.const 1096
        i32.load
        local.get $3
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
      local.get $7
      i32.const 1096
      i32.load
      i32.eq
      if $if_37
        i32.const 1084
        local.get $1
        i32.const 1084
        i32.load
        i32.add
        local.tee $0
        i32.store
        i32.const 1096
        local.get $5
        i32.store
        local.get $3
        local.get $0
        i32.const 1
        i32.or
        i32.store offset=4
        local.get $0
        local.get $5
        i32.add
        local.get $0
        i32.store
        return
      end ;; $if_37
      local.get $1
      local.get $0
      i32.const -8
      i32.and
      i32.add
      local.set $4
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
          i32.const 1116
          i32.add
          local.tee $0
          i32.ne
          if $if_39
            i32.const 1092
            i32.load
            local.get $2
            i32.gt_u
            if $if_40
              call $_abort
            end ;; $if_40
            local.get $2
            i32.load offset=12
            local.get $7
            i32.ne
            if $if_41
              call $_abort
            end ;; $if_41
          end ;; $if_39
          local.get $1
          local.get $2
          i32.eq
          if $if_42
            i32.const 1076
            i32.const 1076
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
            i32.const 1092
            i32.load
            local.get $1
            i32.gt_u
            if $if_44
              call $_abort
            end ;; $if_44
            local.get $7
            local.get $1
            i32.const 8
            i32.add
            local.tee $0
            i32.load
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
          local.get $7
          i32.load offset=12
          local.tee $0
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
              i32.const 1092
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
                local.set $10
              end ;; $if_49
            end ;; $block_3
          else
            i32.const 1092
            i32.load
            local.get $7
            i32.load offset=8
            local.tee $1
            i32.gt_u
            if $if_50
              call $_abort
            end ;; $if_50
            local.get $1
            i32.const 12
            i32.add
            local.tee $2
            i32.load
            local.get $7
            i32.ne
            if $if_51
              call $_abort
            end ;; $if_51
            local.get $7
            local.get $0
            i32.const 8
            i32.add
            local.tee $6
            i32.load
            i32.eq
            if $if_52
              local.get $2
              local.get $0
              i32.store
              local.get $6
              local.get $1
              i32.store
              local.get $0
              local.set $10
            else
              call $_abort
            end ;; $if_52
          end ;; $if_46
          local.get $8
          if $if_53
            local.get $7
            local.get $7
            i32.load offset=28
            local.tee $0
            i32.const 2
            i32.shl
            i32.const 1380
            i32.add
            local.tee $1
            i32.load
            i32.eq
            if $if_54
              local.get $1
              local.get $10
              i32.store
              local.get $10
              i32.eqz
              if $if_55
                i32.const 1080
                i32.const 1080
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
              i32.const 1092
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
                local.get $7
                local.get $0
                i32.load
                i32.eq
                select
                local.get $10
                i32.store
                local.get $10
                i32.eqz
                br_if $block_2
              end ;; $if_56
            end ;; $if_54
            i32.const 1092
            i32.load
            local.tee $1
            local.get $10
            i32.gt_u
            if $if_57
              call $_abort
            end ;; $if_57
            local.get $10
            local.get $8
            i32.store offset=24
            local.get $7
            i32.const 16
            i32.add
            local.tee $2
            i32.load
            local.tee $0
            if $if_58
              local.get $1
              local.get $0
              i32.gt_u
              if $if_59
                call $_abort
              else
                local.get $10
                local.get $0
                i32.store offset=16
                local.get $0
                local.get $10
                i32.store offset=24
              end ;; $if_59
            end ;; $if_58
            local.get $2
            i32.load offset=4
            local.tee $0
            if $if_60
              i32.const 1092
              i32.load
              local.get $0
              i32.gt_u
              if $if_61
                call $_abort
              else
                local.get $10
                local.get $0
                i32.store offset=20
                local.get $0
                local.get $10
                i32.store offset=24
              end ;; $if_61
            end ;; $if_60
          end ;; $if_53
        end ;; $if_38
      end ;; $block_2
      local.get $3
      local.get $4
      i32.const 1
      i32.or
      i32.store offset=4
      local.get $4
      local.get $5
      i32.add
      local.get $4
      i32.store
      local.get $3
      i32.const 1096
      i32.load
      i32.eq
      if $if_62 (result i32)
        i32.const 1084
        local.get $4
        i32.store
        return
      else
        local.get $4
      end ;; $if_62
    end ;; $if_34
    local.tee $5
    i32.const 3
    i32.shr_u
    local.set $1
    local.get $5
    i32.const 256
    i32.lt_u
    if $if_63
      local.get $1
      i32.const 3
      i32.shl
      i32.const 1116
      i32.add
      local.set $0
      i32.const 1076
      i32.load
      local.tee $5
      i32.const 1
      local.get $1
      i32.shl
      local.tee $1
      i32.and
      if $if_64
        i32.const 1092
        i32.load
        local.get $0
        i32.const 8
        i32.add
        local.tee $1
        i32.load
        local.tee $5
        i32.gt_u
        if $if_65
          call $_abort
        else
          local.get $5
          local.set $15
          local.get $1
          local.set $17
        end ;; $if_65
      else
        i32.const 1076
        local.get $1
        local.get $5
        i32.or
        i32.store
        local.get $0
        local.set $15
        local.get $0
        i32.const 8
        i32.add
        local.set $17
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
    local.get $5
    i32.const 8
    i32.shr_u
    local.tee $0
    if $if_66 (result i32)
      local.get $5
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
        local.tee $1
        i32.shl
        local.tee $4
        i32.const 520192
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 4
        i32.and
        local.set $0
        i32.const 14
        local.get $0
        local.get $1
        i32.or
        local.get $4
        local.get $0
        i32.shl
        local.tee $0
        i32.const 245760
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 2
        i32.and
        local.tee $1
        i32.or
        i32.sub
        local.get $0
        local.get $1
        i32.shl
        i32.const 15
        i32.shr_u
        i32.add
        local.tee $0
        i32.const 1
        i32.shl
        local.get $5
        local.get $0
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
    local.tee $1
    i32.const 2
    i32.shl
    i32.const 1380
    i32.add
    local.set $0
    local.get $3
    local.get $1
    i32.store offset=28
    local.get $3
    i32.const 0
    i32.store offset=20
    local.get $3
    i32.const 0
    i32.store offset=16
    i32.const 1080
    i32.load
    local.tee $4
    i32.const 1
    local.get $1
    i32.shl
    local.tee $2
    i32.and
    if $if_68
      block $block_5
        local.get $5
        local.get $0
        i32.load
        local.tee $0
        i32.load offset=4
        i32.const -8
        i32.and
        i32.eq
        if $if_69
          local.get $0
          local.set $14
        else
          block $block_6
            local.get $5
            i32.const 0
            i32.const 25
            local.get $1
            i32.const 1
            i32.shr_u
            i32.sub
            local.get $1
            i32.const 31
            i32.eq
            select
            i32.shl
            local.set $4
            loop $loop_1
              local.get $0
              i32.const 16
              i32.add
              local.get $4
              i32.const 31
              i32.shr_u
              i32.const 2
              i32.shl
              i32.add
              local.tee $2
              i32.load
              local.tee $1
              if $if_70
                local.get $4
                i32.const 1
                i32.shl
                local.set $4
                local.get $5
                local.get $1
                i32.load offset=4
                i32.const -8
                i32.and
                i32.eq
                if $if_71
                  local.get $1
                  local.set $14
                  br $block_6
                else
                  local.get $1
                  local.set $0
                  br $loop_1
                end ;; $if_71
                unreachable
              end ;; $if_70
            end ;; $loop_1
            i32.const 1092
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
        i32.const 1092
        i32.load
        local.tee $0
        local.get $14
        i32.le_u
        local.get $0
        local.get $14
        i32.const 8
        i32.add
        local.tee $1
        i32.load
        local.tee $0
        i32.le_u
        i32.and
        if $if_73
          local.get $0
          local.get $3
          i32.store offset=12
          local.get $1
          local.get $3
          i32.store
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
      i32.const 1080
      local.get $2
      local.get $4
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
    i32.const 1108
    i32.const 1108
    i32.load
    i32.const -1
    i32.add
    local.tee $0
    i32.store
    local.get $0
    if $if_74
      return
    end ;; $if_74
    i32.const 1532
    local.set $0
    loop $loop_2
      local.get $0
      i32.load
      local.tee $3
      i32.const 8
      i32.add
      local.set $0
      local.get $3
      br_if $loop_2
    end ;; $loop_2
    i32.const 1108
    i32.const -1
    i32.store
    )
  
  (func $__Znwm (type $4)
    (param $0 i32)
    (result i32)
    (local $1 i32)
    local.get $0
    i32.const 1
    local.get $0
    select
    local.set $1
    loop $loop
      local.get $1
      call $_malloc
      local.tee $0
      i32.eqz
      if $if
        call $__ZSt15get_new_handlerv
        if $if_0
          i32.const 0
          call_indirect $9 (type $0)
          br $loop
        else
          i32.const 0
          local.set $0
        end ;; $if_0
      end ;; $if
    end ;; $loop
    local.get $0
    )
  
  (func $__ZdlPv (type $1)
    (param $0 i32)
    local.get $0
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
    local.get $2
    if $if
      local.get $0
      local.get $1
      local.get $2
      call $_memcpy
      drop
    end ;; $if
    local.get $0
    )
  
  (func $__ZNSt3__211char_traitsIcE6assignERcRKc (type $6)
    (param $0 i32)
    (param $1 i32)
    local.get $0
    local.get $1
    i32.load8_s
    i32.store8
    )
  
  (func $__ZNSt3__211char_traitsIcE6lengthEPKc (type $4)
    (param $0 i32)
    (result i32)
    local.get $0
    call $_strlen
    )
  
  (func $__ZNSt3__211char_traitsIcE4moveEPcPKcm (type $3)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    local.get $2
    if $if
      local.get $0
      local.get $1
      local.get $2
      call $_memmove
      drop
    end ;; $if
    local.get $0
    )
  
  (func $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE21__grow_by_and_replaceEmmmmmmPKc (type $7)
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
    global.get $16
    local.set $7
    global.get $16
    i32.const 16
    i32.add
    global.set $16
    i32.const -18
    local.get $1
    i32.sub
    local.get $2
    i32.lt_u
    if $if
      call $__ZNKSt3__221__basic_string_commonILb1EE20__throw_length_errorEv
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
    local.set $8
    local.get $1
    i32.const 2147483623
    i32.lt_u
    if $if_1 (result i32)
      i32.const 11
      local.get $1
      i32.const 1
      i32.shl
      local.tee $9
      local.get $1
      local.get $2
      i32.add
      local.tee $2
      local.get $2
      local.get $9
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
    local.tee $9
    call $__Znwm
    local.set $2
    local.get $4
    if $if_2
      local.get $2
      local.get $8
      local.get $4
      call $__ZNSt3__211char_traitsIcE4copyEPcPKcm
      drop
    end ;; $if_2
    local.get $5
    if $if_3
      local.get $2
      local.get $4
      i32.add
      local.get $6
      local.get $5
      call $__ZNSt3__211char_traitsIcE4copyEPcPKcm
      drop
    end ;; $if_3
    local.get $3
    local.get $4
    i32.sub
    local.tee $6
    if $if_4
      local.get $5
      local.get $2
      local.get $4
      i32.add
      i32.add
      local.get $4
      local.get $8
      i32.add
      local.get $6
      call $__ZNSt3__211char_traitsIcE4copyEPcPKcm
      drop
    end ;; $if_4
    local.get $1
    i32.const 10
    i32.ne
    if $if_5
      local.get $8
      call $__ZdlPv
    end ;; $if_5
    local.get $0
    local.get $2
    i32.store
    local.get $0
    local.get $9
    i32.const -2147483648
    i32.or
    i32.store offset=8
    local.get $0
    local.get $3
    local.get $5
    i32.add
    local.tee $0
    i32.store offset=4
    local.get $7
    i32.const 0
    i32.store8
    local.get $0
    local.get $2
    i32.add
    local.get $7
    call $__ZNSt3__211char_traitsIcE6assignERcRKc
    local.get $7
    global.set $16
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
    global.get $16
    local.set $5
    global.get $16
    i32.const 16
    i32.add
    global.set $16
    local.get $0
    i32.const 11
    i32.add
    local.tee $7
    i32.load8_s
    local.tee $4
    i32.const 0
    i32.lt_s
    local.tee $6
    if $if (result i32)
      local.get $0
      i32.load offset=4
      local.set $3
      local.get $0
      i32.load offset=8
      i32.const 2147483647
      i32.and
      i32.const -1
      i32.add
    else
      local.get $4
      i32.const 255
      i32.and
      local.set $3
      i32.const 10
    end ;; $if
    local.tee $4
    local.get $3
    i32.sub
    local.get $2
    i32.lt_u
    if $if_0
      local.get $0
      local.get $4
      local.get $2
      local.get $3
      i32.add
      local.get $4
      i32.sub
      local.get $3
      local.get $3
      local.get $2
      local.get $1
      call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE21__grow_by_and_replaceEmmmmmmPKc
    else
      local.get $2
      if $if_1
        local.get $3
        local.get $6
        if $if_2 (result i32)
          local.get $0
          i32.load
        else
          local.get $0
        end ;; $if_2
        local.tee $6
        i32.add
        local.get $1
        local.get $2
        call $__ZNSt3__211char_traitsIcE4copyEPcPKcm
        drop
        local.get $2
        local.get $3
        i32.add
        local.set $1
        local.get $7
        i32.load8_s
        i32.const 0
        i32.lt_s
        if $if_3
          local.get $0
          local.get $1
          i32.store offset=4
        else
          local.get $7
          local.get $1
          i32.store8
        end ;; $if_3
        local.get $5
        i32.const 0
        i32.store8
        local.get $1
        local.get $6
        i32.add
        local.get $5
        call $__ZNSt3__211char_traitsIcE6assignERcRKc
      end ;; $if_1
    end ;; $if_0
    local.get $5
    global.set $16
    local.get $0
    )
  
  (func $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6appendEPKc (type $4)
    (param $0 i32)
    (result i32)
    local.get $0
    i32.const 1041
    i32.const 1041
    call $__ZNSt3__211char_traitsIcE6lengthEPKc
    call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6appendEPKcm
    )
  
  (func $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6insertEmPKcm (type $8)
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
    global.get $16
    local.set $7
    global.get $16
    i32.const 16
    i32.add
    global.set $16
    local.get $0
    i32.const 11
    i32.add
    local.tee $8
    i32.load8_s
    local.tee $4
    i32.const 0
    i32.lt_s
    local.tee $5
    if $if (result i32)
      local.get $0
      i32.load offset=4
    else
      local.get $4
      i32.const 255
      i32.and
    end ;; $if
    local.tee $6
    local.get $1
    i32.lt_u
    if $if_0
      call $__ZNKSt3__221__basic_string_commonILb1EE20__throw_length_errorEv
    end ;; $if_0
    local.get $5
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
    local.tee $4
    local.get $6
    i32.sub
    local.get $3
    i32.lt_u
    if $if_2
      local.get $0
      local.get $4
      local.get $3
      local.get $6
      i32.add
      local.get $4
      i32.sub
      local.get $6
      local.get $1
      local.get $3
      local.get $2
      call $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE21__grow_by_and_replaceEmmmmmmPKc
    else
      local.get $3
      if $if_3
        local.get $1
        local.get $5
        if $if_4 (result i32)
          local.get $0
          i32.load
        else
          local.get $0
        end ;; $if_4
        local.tee $4
        i32.add
        local.set $5
        local.get $6
        local.get $1
        i32.sub
        local.tee $1
        if $if_5
          local.get $2
          local.get $3
          i32.add
          local.get $2
          local.get $5
          local.get $2
          i32.le_u
          local.get $4
          local.get $6
          i32.add
          local.get $2
          i32.gt_u
          i32.and
          select
          local.set $2
          local.get $3
          local.get $5
          i32.add
          local.get $5
          local.get $1
          call $__ZNSt3__211char_traitsIcE4moveEPcPKcm
          drop
        end ;; $if_5
        local.get $5
        local.get $2
        local.get $3
        call $__ZNSt3__211char_traitsIcE4moveEPcPKcm
        drop
        local.get $3
        local.get $6
        i32.add
        local.set $1
        local.get $8
        i32.load8_s
        i32.const 0
        i32.lt_s
        if $if_6
          local.get $0
          local.get $1
          i32.store offset=4
        else
          local.get $8
          local.get $1
          i32.store8
        end ;; $if_6
        local.get $7
        i32.const 0
        i32.store8
        local.get $1
        local.get $4
        i32.add
        local.get $7
        call $__ZNSt3__211char_traitsIcE6assignERcRKc
      end ;; $if_3
    end ;; $if_2
    local.get $7
    global.set $16
    local.get $0
    )
  
  (func $__ZNSt3__212basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE6insertEmPKc (type $4)
    (param $0 i32)
    (result i32)
    local.get $0
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
    local.tee $0
    i32.store
    local.get $0
    )
  
  (func $_memcpy (type $3)
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
  
  (func $_memmove (type $3)
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
  
  (func $_memset (type $3)
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
    local.set $1
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
          local.get $1
          i32.store8
          local.get $0
          i32.const 1
          i32.add
          local.set $0
          br $loop
        end ;; $if_0
      end ;; $loop
      local.get $4
      i32.const -4
      i32.and
      local.tee $5
      i32.const -64
      i32.add
      local.set $6
      local.get $1
      i32.const 8
      i32.shl
      local.get $1
      i32.or
      local.get $1
      i32.const 16
      i32.shl
      i32.or
      local.get $1
      i32.const 24
      i32.shl
      i32.or
      local.set $3
      loop $loop_0
        local.get $0
        local.get $6
        i32.le_s
        if $if_1
          local.get $0
          local.get $3
          i32.store
          local.get $0
          local.get $3
          i32.store offset=4
          local.get $0
          local.get $3
          i32.store offset=8
          local.get $0
          local.get $3
          i32.store offset=12
          local.get $0
          local.get $3
          i32.store offset=16
          local.get $0
          local.get $3
          i32.store offset=20
          local.get $0
          local.get $3
          i32.store offset=24
          local.get $0
          local.get $3
          i32.store offset=28
          local.get $0
          local.get $3
          i32.store offset=32
          local.get $0
          local.get $3
          i32.store offset=36
          local.get $0
          local.get $3
          i32.store offset=40
          local.get $0
          local.get $3
          i32.store offset=44
          local.get $0
          local.get $3
          i32.store offset=48
          local.get $0
          local.get $3
          i32.store offset=52
          local.get $0
          local.get $3
          i32.store offset=56
          local.get $0
          local.get $3
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
          local.get $3
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
        local.get $1
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
  
  (func $_sbrk (type $4)
    (param $0 i32)
    (result i32)
    (local $1 i32)
    (local $2 i32)
    local.get $0
    global.get $13
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
      call $abortOnCannotGrowMemory
      drop
      i32.const 12
      call $___setErrNo
      i32.const -1
      return
    end ;; $if
    local.get $1
    call $_emscripten_get_heap_size
    i32.le_s
    if $if_0
      global.get $13
      local.get $1
      i32.store
    else
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
    local.get $2
    )
  
  (func $dynCall_v (type $1)
    (param $0 i32)
    i32.const 0
    call_indirect $9 (type $0)
    )
  
  (func $b0 (type $0)
    i32.const 0
    call $abort
    )
  ;; User section "emscripten_metadata":
    ;; "\00\00\00\01\b0\0e\01"
  )