(module
 (type $FUNCSIG$i (func (result i32)))
 (type $FUNCSIG$vi (func (param i32)))
 (type $FUNCSIG$v (func))
 (type $FUNCSIG$iiii (func (param i32 i32 i32) (result i32)))
 (type $FUNCSIG$ii (func (param i32) (result i32)))
 (import "env" "memory" (memory $memory 256 256))
 (import "env" "DYNAMICTOP_PTR" (global $DYNAMICTOP_PTR$asm2wasm$import i32))
 (import "env" "abortOnCannotGrowMemory" (func $abortOnCannotGrowMemory (result i32)))
 (import "env" "___setErrNo" (func $___setErrNo (param i32)))
 (import "env" "_abort" (func $_abort))
 (import "env" "_emscripten_get_heap_size" (func $_emscripten_get_heap_size (result i32)))
 (import "env" "_emscripten_memcpy_big" (func $_emscripten_memcpy_big (param i32 i32 i32) (result i32)))
 (import "env" "_emscripten_resize_heap" (func $_emscripten_resize_heap (param i32) (result i32)))
 (global $DYNAMICTOP_PTR (mut i32) (global.get $DYNAMICTOP_PTR$asm2wasm$import))
 (global $__THREW__ (mut i32) (i32.const 0))
 (global $threwValue (mut i32) (i32.const 0))
 (global $STACKTOP (mut i32) (i32.const 2816))
 (global $STACK_MAX (mut i32) (i32.const 5245696))
 (export "___errno_location" (func $___errno_location))
 (export "_free" (func $_free))
 (export "_malloc" (func $_malloc))
 (export "_memcpy" (func $_memcpy))
 (export "_memset" (func $_memset))
 (export "_proxy_onStart" (func $_proxy_onStart))
 (export "_sbrk" (func $_sbrk))
 (export "establishStackSpace" (func $establishStackSpace))
 (export "setThrew" (func $setThrew))
 (export "stackAlloc" (func $stackAlloc))
 (export "stackRestore" (func $stackRestore))
 (export "stackSave" (func $stackSave))
 (func $stackAlloc (; 6 ;) (; has Stack IR ;) (param $0 i32) (result i32)
  (local $1 i32)
  (local.set $1
   (global.get $STACKTOP)
  )
  (global.set $STACKTOP
   (i32.add
    (local.get $0)
    (global.get $STACKTOP)
   )
  )
  (global.set $STACKTOP
   (i32.and
    (i32.add
     (global.get $STACKTOP)
     (i32.const 15)
    )
    (i32.const -16)
   )
  )
  (local.get $1)
 )
 (func $stackSave (; 7 ;) (; has Stack IR ;) (result i32)
  (global.get $STACKTOP)
 )
 (func $stackRestore (; 8 ;) (; has Stack IR ;) (param $0 i32)
  (global.set $STACKTOP
   (local.get $0)
  )
 )
 (func $establishStackSpace (; 9 ;) (; has Stack IR ;) (param $0 i32) (param $1 i32)
  (global.set $STACKTOP
   (local.get $0)
  )
  (global.set $STACK_MAX
   (local.get $1)
  )
 )
 (func $setThrew (; 10 ;) (; has Stack IR ;) (param $0 i32) (param $1 i32)
  (if
   (i32.eqz
    (global.get $__THREW__)
   )
   (block
    (global.set $__THREW__
     (local.get $0)
    )
    (global.set $threwValue
     (local.get $1)
    )
   )
  )
 )
 (func $_proxy_onStart (; 11 ;) (; has Stack IR ;)
  (nop)
 )
 (func $_malloc (; 12 ;) (; has Stack IR ;) (param $0 i32) (result i32)
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
  (local.set $14
   (global.get $STACKTOP)
  )
  (global.set $STACKTOP
   (i32.add
    (global.get $STACKTOP)
    (i32.const 16)
   )
  )
  (local.set $3
   (if (result i32)
    (i32.lt_u
     (local.get $0)
     (i32.const 245)
    )
    (block (result i32)
     (if
      (i32.and
       (local.tee $3
        (i32.shr_u
         (local.tee $7
          (i32.load
           (i32.const 1024)
          )
         )
         (local.tee $0
          (i32.shr_u
           (local.tee $2
            (select
             (i32.const 16)
             (i32.and
              (i32.add
               (local.get $0)
               (i32.const 11)
              )
              (i32.const -8)
             )
             (i32.lt_u
              (local.get $0)
              (i32.const 11)
             )
            )
           )
           (i32.const 3)
          )
         )
        )
       )
       (i32.const 3)
      )
      (block
       (local.set $0
        (i32.load
         (local.tee $6
          (i32.add
           (local.tee $3
            (i32.load
             (local.tee $4
              (i32.add
               (local.tee $2
                (i32.add
                 (i32.shl
                  (local.tee $1
                   (i32.add
                    (i32.xor
                     (i32.and
                      (local.get $3)
                      (i32.const 1)
                     )
                     (i32.const 1)
                    )
                    (local.get $0)
                   )
                  )
                  (i32.const 3)
                 )
                 (i32.const 1064)
                )
               )
               (i32.const 8)
              )
             )
            )
           )
           (i32.const 8)
          )
         )
        )
       )
       (if
        (i32.eq
         (local.get $0)
         (local.get $2)
        )
        (i32.store
         (i32.const 1024)
         (i32.and
          (i32.xor
           (i32.shl
            (i32.const 1)
            (local.get $1)
           )
           (i32.const -1)
          )
          (local.get $7)
         )
        )
        (block
         (if
          (i32.gt_u
           (i32.load
            (i32.const 1040)
           )
           (local.get $0)
          )
          (call $_abort)
         )
         (if
          (i32.eq
           (local.get $3)
           (i32.load
            (local.tee $5
             (i32.add
              (local.get $0)
              (i32.const 12)
             )
            )
           )
          )
          (block
           (i32.store
            (local.get $5)
            (local.get $2)
           )
           (i32.store
            (local.get $4)
            (local.get $0)
           )
          )
          (call $_abort)
         )
        )
       )
       (i32.store offset=4
        (local.get $3)
        (i32.or
         (local.tee $0
          (i32.shl
           (local.get $1)
           (i32.const 3)
          )
         )
         (i32.const 3)
        )
       )
       (i32.store
        (local.tee $0
         (i32.add
          (i32.add
           (local.get $0)
           (local.get $3)
          )
          (i32.const 4)
         )
        )
        (i32.or
         (i32.load
          (local.get $0)
         )
         (i32.const 1)
        )
       )
       (global.set $STACKTOP
        (local.get $14)
       )
       (return
        (local.get $6)
       )
      )
     )
     (if (result i32)
      (i32.gt_u
       (local.get $2)
       (local.tee $13
        (i32.load
         (i32.const 1032)
        )
       )
      )
      (block (result i32)
       (if
        (local.get $3)
        (block
         (local.set $3
          (i32.load
           (local.tee $10
            (i32.add
             (local.tee $0
              (i32.load
               (local.tee $9
                (i32.add
                 (local.tee $5
                  (i32.add
                   (i32.shl
                    (local.tee $1
                     (i32.add
                      (i32.or
                       (i32.or
                        (i32.or
                         (i32.or
                          (local.tee $3
                           (i32.and
                            (i32.shr_u
                             (local.tee $0
                              (i32.add
                               (i32.and
                                (local.tee $0
                                 (i32.and
                                  (i32.shl
                                   (local.get $3)
                                   (local.get $0)
                                  )
                                  (i32.or
                                   (local.tee $0
                                    (i32.shl
                                     (i32.const 2)
                                     (local.get $0)
                                    )
                                   )
                                   (i32.sub
                                    (i32.const 0)
                                    (local.get $0)
                                   )
                                  )
                                 )
                                )
                                (i32.sub
                                 (i32.const 0)
                                 (local.get $0)
                                )
                               )
                               (i32.const -1)
                              )
                             )
                             (i32.const 12)
                            )
                            (i32.const 16)
                           )
                          )
                          (local.tee $3
                           (i32.and
                            (i32.shr_u
                             (local.tee $0
                              (i32.shr_u
                               (local.get $0)
                               (local.get $3)
                              )
                             )
                             (i32.const 5)
                            )
                            (i32.const 8)
                           )
                          )
                         )
                         (local.tee $3
                          (i32.and
                           (i32.shr_u
                            (local.tee $0
                             (i32.shr_u
                              (local.get $0)
                              (local.get $3)
                             )
                            )
                            (i32.const 2)
                           )
                           (i32.const 4)
                          )
                         )
                        )
                        (local.tee $3
                         (i32.and
                          (i32.shr_u
                           (local.tee $0
                            (i32.shr_u
                             (local.get $0)
                             (local.get $3)
                            )
                           )
                           (i32.const 1)
                          )
                          (i32.const 2)
                         )
                        )
                       )
                       (local.tee $3
                        (i32.and
                         (i32.shr_u
                          (local.tee $0
                           (i32.shr_u
                            (local.get $0)
                            (local.get $3)
                           )
                          )
                          (i32.const 1)
                         )
                         (i32.const 1)
                        )
                       )
                      )
                      (i32.shr_u
                       (local.get $0)
                       (local.get $3)
                      )
                     )
                    )
                    (i32.const 3)
                   )
                   (i32.const 1064)
                  )
                 )
                 (i32.const 8)
                )
               )
              )
             )
             (i32.const 8)
            )
           )
          )
         )
         (if
          (i32.eq
           (local.get $3)
           (local.get $5)
          )
          (i32.store
           (i32.const 1024)
           (local.tee $4
            (i32.and
             (i32.xor
              (i32.shl
               (i32.const 1)
               (local.get $1)
              )
              (i32.const -1)
             )
             (local.get $7)
            )
           )
          )
          (block
           (if
            (i32.gt_u
             (i32.load
              (i32.const 1040)
             )
             (local.get $3)
            )
            (call $_abort)
           )
           (if
            (i32.eq
             (local.get $0)
             (i32.load
              (local.tee $11
               (i32.add
                (local.get $3)
                (i32.const 12)
               )
              )
             )
            )
            (block
             (i32.store
              (local.get $11)
              (local.get $5)
             )
             (i32.store
              (local.get $9)
              (local.get $3)
             )
             (local.set $4
              (local.get $7)
             )
            )
            (call $_abort)
           )
          )
         )
         (i32.store offset=4
          (local.get $0)
          (i32.or
           (local.get $2)
           (i32.const 3)
          )
         )
         (i32.store offset=4
          (local.tee $7
           (i32.add
            (local.get $0)
            (local.get $2)
           )
          )
          (i32.or
           (local.tee $5
            (i32.sub
             (local.tee $3
              (i32.shl
               (local.get $1)
               (i32.const 3)
              )
             )
             (local.get $2)
            )
           )
           (i32.const 1)
          )
         )
         (i32.store
          (i32.add
           (local.get $0)
           (local.get $3)
          )
          (local.get $5)
         )
         (if
          (local.get $13)
          (block
           (local.set $2
            (i32.load
             (i32.const 1044)
            )
           )
           (local.set $0
            (i32.add
             (i32.shl
              (local.tee $3
               (i32.shr_u
                (local.get $13)
                (i32.const 3)
               )
              )
              (i32.const 3)
             )
             (i32.const 1064)
            )
           )
           (if
            (i32.and
             (local.tee $3
              (i32.shl
               (i32.const 1)
               (local.get $3)
              )
             )
             (local.get $4)
            )
            (if
             (i32.gt_u
              (i32.load
               (i32.const 1040)
              )
              (local.tee $1
               (i32.load
                (local.tee $3
                 (i32.add
                  (local.get $0)
                  (i32.const 8)
                 )
                )
               )
              )
             )
             (call $_abort)
             (block
              (local.set $6
               (local.get $1)
              )
              (local.set $12
               (local.get $3)
              )
             )
            )
            (block
             (i32.store
              (i32.const 1024)
              (i32.or
               (local.get $3)
               (local.get $4)
              )
             )
             (local.set $6
              (local.get $0)
             )
             (local.set $12
              (i32.add
               (local.get $0)
               (i32.const 8)
              )
             )
            )
           )
           (i32.store
            (local.get $12)
            (local.get $2)
           )
           (i32.store offset=12
            (local.get $6)
            (local.get $2)
           )
           (i32.store offset=8
            (local.get $2)
            (local.get $6)
           )
           (i32.store offset=12
            (local.get $2)
            (local.get $0)
           )
          )
         )
         (i32.store
          (i32.const 1032)
          (local.get $5)
         )
         (i32.store
          (i32.const 1044)
          (local.get $7)
         )
         (global.set $STACKTOP
          (local.get $14)
         )
         (return
          (local.get $10)
         )
        )
       )
       (if (result i32)
        (local.tee $12
         (i32.load
          (i32.const 1028)
         )
        )
        (block
         (local.set $0
          (local.tee $4
           (i32.load
            (i32.add
             (i32.shl
              (i32.add
               (i32.or
                (i32.or
                 (i32.or
                  (i32.or
                   (local.tee $3
                    (i32.and
                     (i32.shr_u
                      (local.tee $0
                       (i32.add
                        (i32.and
                         (local.get $12)
                         (i32.sub
                          (i32.const 0)
                          (local.get $12)
                         )
                        )
                        (i32.const -1)
                       )
                      )
                      (i32.const 12)
                     )
                     (i32.const 16)
                    )
                   )
                   (local.tee $3
                    (i32.and
                     (i32.shr_u
                      (local.tee $0
                       (i32.shr_u
                        (local.get $0)
                        (local.get $3)
                       )
                      )
                      (i32.const 5)
                     )
                     (i32.const 8)
                    )
                   )
                  )
                  (local.tee $3
                   (i32.and
                    (i32.shr_u
                     (local.tee $0
                      (i32.shr_u
                       (local.get $0)
                       (local.get $3)
                      )
                     )
                     (i32.const 2)
                    )
                    (i32.const 4)
                   )
                  )
                 )
                 (local.tee $3
                  (i32.and
                   (i32.shr_u
                    (local.tee $0
                     (i32.shr_u
                      (local.get $0)
                      (local.get $3)
                     )
                    )
                    (i32.const 1)
                   )
                   (i32.const 2)
                  )
                 )
                )
                (local.tee $3
                 (i32.and
                  (i32.shr_u
                   (local.tee $0
                    (i32.shr_u
                     (local.get $0)
                     (local.get $3)
                    )
                   )
                   (i32.const 1)
                  )
                  (i32.const 1)
                 )
                )
               )
               (i32.shr_u
                (local.get $0)
                (local.get $3)
               )
              )
              (i32.const 2)
             )
             (i32.const 1328)
            )
           )
          )
         )
         (local.set $10
          (i32.sub
           (i32.and
            (i32.load offset=4
             (local.get $4)
            )
            (i32.const -8)
           )
           (local.get $2)
          )
         )
         (loop $while-in
          (block $while-out
           (if
            (local.tee $3
             (i32.load offset=16
              (local.get $0)
             )
            )
            (local.set $0
             (local.get $3)
            )
            (br_if $while-out
             (i32.eqz
              (local.tee $0
               (i32.load offset=20
                (local.get $0)
               )
              )
             )
            )
           )
           (local.set $4
            (select
             (local.get $0)
             (local.get $4)
             (local.tee $6
              (i32.lt_u
               (local.tee $3
                (i32.sub
                 (i32.and
                  (i32.load offset=4
                   (local.get $0)
                  )
                  (i32.const -8)
                 )
                 (local.get $2)
                )
               )
               (local.get $10)
              )
             )
            )
           )
           (local.set $10
            (select
             (local.get $3)
             (local.get $10)
             (local.get $6)
            )
           )
           (br $while-in)
          )
         )
         (if
          (i32.gt_u
           (local.tee $15
            (i32.load
             (i32.const 1040)
            )
           )
           (local.get $4)
          )
          (call $_abort)
         )
         (if
          (i32.le_u
           (local.tee $8
            (i32.add
             (local.get $2)
             (local.get $4)
            )
           )
           (local.get $4)
          )
          (call $_abort)
         )
         (local.set $11
          (i32.load offset=24
           (local.get $4)
          )
         )
         (if
          (i32.eq
           (local.get $4)
           (local.tee $0
            (i32.load offset=12
             (local.get $4)
            )
           )
          )
          (block $do-once4
           (if
            (i32.eqz
             (local.tee $0
              (i32.load
               (local.tee $3
                (i32.add
                 (local.get $4)
                 (i32.const 20)
                )
               )
              )
             )
            )
            (br_if $do-once4
             (i32.eqz
              (local.tee $0
               (i32.load
                (local.tee $3
                 (i32.add
                  (local.get $4)
                  (i32.const 16)
                 )
                )
               )
              )
             )
            )
           )
           (loop $while-in7
            (block $while-out6
             (if
              (i32.eqz
               (local.tee $9
                (i32.load
                 (local.tee $6
                  (i32.add
                   (local.get $0)
                   (i32.const 20)
                  )
                 )
                )
               )
              )
              (br_if $while-out6
               (i32.eqz
                (local.tee $9
                 (i32.load
                  (local.tee $6
                   (i32.add
                    (local.get $0)
                    (i32.const 16)
                   )
                  )
                 )
                )
               )
              )
             )
             (local.set $3
              (local.get $6)
             )
             (local.set $0
              (local.get $9)
             )
             (br $while-in7)
            )
           )
           (if
            (i32.gt_u
             (local.get $15)
             (local.get $3)
            )
            (call $_abort)
            (block
             (i32.store
              (local.get $3)
              (i32.const 0)
             )
             (local.set $1
              (local.get $0)
             )
            )
           )
          )
          (block
           (if
            (i32.gt_u
             (local.get $15)
             (local.tee $3
              (i32.load offset=8
               (local.get $4)
              )
             )
            )
            (call $_abort)
           )
           (if
            (i32.ne
             (i32.load
              (local.tee $6
               (i32.add
                (local.get $3)
                (i32.const 12)
               )
              )
             )
             (local.get $4)
            )
            (call $_abort)
           )
           (if
            (i32.eq
             (local.get $4)
             (i32.load
              (local.tee $9
               (i32.add
                (local.get $0)
                (i32.const 8)
               )
              )
             )
            )
            (block
             (i32.store
              (local.get $6)
              (local.get $0)
             )
             (i32.store
              (local.get $9)
              (local.get $3)
             )
             (local.set $1
              (local.get $0)
             )
            )
            (call $_abort)
           )
          )
         )
         (if
          (local.get $11)
          (block $label$break$L78
           (if
            (i32.eq
             (local.get $4)
             (i32.load
              (local.tee $3
               (i32.add
                (i32.shl
                 (local.tee $0
                  (i32.load offset=28
                   (local.get $4)
                  )
                 )
                 (i32.const 2)
                )
                (i32.const 1328)
               )
              )
             )
            )
            (block
             (i32.store
              (local.get $3)
              (local.get $1)
             )
             (if
              (i32.eqz
               (local.get $1)
              )
              (block
               (i32.store
                (i32.const 1028)
                (i32.and
                 (local.get $12)
                 (i32.xor
                  (i32.shl
                   (i32.const 1)
                   (local.get $0)
                  )
                  (i32.const -1)
                 )
                )
               )
               (br $label$break$L78)
              )
             )
            )
            (if
             (i32.gt_u
              (i32.load
               (i32.const 1040)
              )
              (local.get $11)
             )
             (call $_abort)
             (block
              (i32.store
               (select
                (local.tee $0
                 (i32.add
                  (local.get $11)
                  (i32.const 16)
                 )
                )
                (i32.add
                 (local.get $11)
                 (i32.const 20)
                )
                (i32.eq
                 (local.get $4)
                 (i32.load
                  (local.get $0)
                 )
                )
               )
               (local.get $1)
              )
              (br_if $label$break$L78
               (i32.eqz
                (local.get $1)
               )
              )
             )
            )
           )
           (if
            (i32.gt_u
             (local.tee $3
              (i32.load
               (i32.const 1040)
              )
             )
             (local.get $1)
            )
            (call $_abort)
           )
           (i32.store offset=24
            (local.get $1)
            (local.get $11)
           )
           (if
            (local.tee $0
             (i32.load offset=16
              (local.get $4)
             )
            )
            (if
             (i32.gt_u
              (local.get $3)
              (local.get $0)
             )
             (call $_abort)
             (block
              (i32.store offset=16
               (local.get $1)
               (local.get $0)
              )
              (i32.store offset=24
               (local.get $0)
               (local.get $1)
              )
             )
            )
           )
           (if
            (local.tee $0
             (i32.load offset=20
              (local.get $4)
             )
            )
            (if
             (i32.gt_u
              (i32.load
               (i32.const 1040)
              )
              (local.get $0)
             )
             (call $_abort)
             (block
              (i32.store offset=20
               (local.get $1)
               (local.get $0)
              )
              (i32.store offset=24
               (local.get $0)
               (local.get $1)
              )
             )
            )
           )
          )
         )
         (if
          (i32.lt_u
           (local.get $10)
           (i32.const 16)
          )
          (block
           (i32.store offset=4
            (local.get $4)
            (i32.or
             (local.tee $0
              (i32.add
               (local.get $2)
               (local.get $10)
              )
             )
             (i32.const 3)
            )
           )
           (i32.store
            (local.tee $0
             (i32.add
              (i32.add
               (local.get $0)
               (local.get $4)
              )
              (i32.const 4)
             )
            )
            (i32.or
             (i32.load
              (local.get $0)
             )
             (i32.const 1)
            )
           )
          )
          (block
           (i32.store offset=4
            (local.get $4)
            (i32.or
             (local.get $2)
             (i32.const 3)
            )
           )
           (i32.store offset=4
            (local.get $8)
            (i32.or
             (local.get $10)
             (i32.const 1)
            )
           )
           (i32.store
            (i32.add
             (local.get $8)
             (local.get $10)
            )
            (local.get $10)
           )
           (if
            (local.get $13)
            (block
             (local.set $2
              (i32.load
               (i32.const 1044)
              )
             )
             (local.set $0
              (i32.add
               (i32.shl
                (local.tee $3
                 (i32.shr_u
                  (local.get $13)
                  (i32.const 3)
                 )
                )
                (i32.const 3)
               )
               (i32.const 1064)
              )
             )
             (if
              (i32.and
               (local.tee $3
                (i32.shl
                 (i32.const 1)
                 (local.get $3)
                )
               )
               (local.get $7)
              )
              (if
               (i32.gt_u
                (i32.load
                 (i32.const 1040)
                )
                (local.tee $1
                 (i32.load
                  (local.tee $3
                   (i32.add
                    (local.get $0)
                    (i32.const 8)
                   )
                  )
                 )
                )
               )
               (call $_abort)
               (block
                (local.set $5
                 (local.get $1)
                )
                (local.set $16
                 (local.get $3)
                )
               )
              )
              (block
               (i32.store
                (i32.const 1024)
                (i32.or
                 (local.get $3)
                 (local.get $7)
                )
               )
               (local.set $5
                (local.get $0)
               )
               (local.set $16
                (i32.add
                 (local.get $0)
                 (i32.const 8)
                )
               )
              )
             )
             (i32.store
              (local.get $16)
              (local.get $2)
             )
             (i32.store offset=12
              (local.get $5)
              (local.get $2)
             )
             (i32.store offset=8
              (local.get $2)
              (local.get $5)
             )
             (i32.store offset=12
              (local.get $2)
              (local.get $0)
             )
            )
           )
           (i32.store
            (i32.const 1032)
            (local.get $10)
           )
           (i32.store
            (i32.const 1044)
            (local.get $8)
           )
          )
         )
         (global.set $STACKTOP
          (local.get $14)
         )
         (return
          (i32.add
           (local.get $4)
           (i32.const 8)
          )
         )
        )
        (local.get $2)
       )
      )
      (local.get $2)
     )
    )
    (if (result i32)
     (i32.gt_u
      (local.get $0)
      (i32.const -65)
     )
     (i32.const -1)
     (block $do-once (result i32)
      (local.set $4
       (i32.and
        (local.tee $0
         (i32.add
          (local.get $0)
          (i32.const 11)
         )
        )
        (i32.const -8)
       )
      )
      (if (result i32)
       (local.tee $6
        (i32.load
         (i32.const 1028)
        )
       )
       (block (result i32)
        (local.set $18
         (if (result i32)
          (local.tee $0
           (i32.shr_u
            (local.get $0)
            (i32.const 8)
           )
          )
          (if (result i32)
           (i32.gt_u
            (local.get $4)
            (i32.const 16777215)
           )
           (i32.const 31)
           (block (result i32)
            (local.set $0
             (i32.and
              (i32.shr_u
               (i32.add
                (local.tee $2
                 (i32.shl
                  (local.get $0)
                  (local.tee $1
                   (i32.and
                    (i32.shr_u
                     (i32.add
                      (local.get $0)
                      (i32.const 1048320)
                     )
                     (i32.const 16)
                    )
                    (i32.const 8)
                   )
                  )
                 )
                )
                (i32.const 520192)
               )
               (i32.const 16)
              )
              (i32.const 4)
             )
            )
            (i32.or
             (i32.shl
              (local.tee $0
               (i32.add
                (i32.sub
                 (i32.const 14)
                 (i32.or
                  (i32.or
                   (local.get $0)
                   (local.get $1)
                  )
                  (local.tee $1
                   (i32.and
                    (i32.shr_u
                     (i32.add
                      (local.tee $0
                       (i32.shl
                        (local.get $2)
                        (local.get $0)
                       )
                      )
                      (i32.const 245760)
                     )
                     (i32.const 16)
                    )
                    (i32.const 2)
                   )
                  )
                 )
                )
                (i32.shr_u
                 (i32.shl
                  (local.get $0)
                  (local.get $1)
                 )
                 (i32.const 15)
                )
               )
              )
              (i32.const 1)
             )
             (i32.and
              (i32.shr_u
               (local.get $4)
               (i32.add
                (local.get $0)
                (i32.const 7)
               )
              )
              (i32.const 1)
             )
            )
           )
          )
          (i32.const 0)
         )
        )
        (local.set $2
         (i32.sub
          (i32.const 0)
          (local.get $4)
         )
        )
        (block $__rjto$1
         (block $__rjti$1
          (local.set $0
           (if (result i32)
            (local.tee $0
             (i32.load
              (i32.add
               (i32.shl
                (local.get $18)
                (i32.const 2)
               )
               (i32.const 1328)
              )
             )
            )
            (block (result i32)
             (local.set $1
              (i32.const 0)
             )
             (local.set $12
              (i32.shl
               (local.get $4)
               (select
                (i32.const 0)
                (i32.sub
                 (i32.const 25)
                 (i32.shr_u
                  (local.get $18)
                  (i32.const 1)
                 )
                )
                (i32.eq
                 (local.get $18)
                 (i32.const 31)
                )
               )
              )
             )
             (loop $while-in15 (result i32)
              (if
               (i32.lt_u
                (local.tee $16
                 (i32.sub
                  (i32.and
                   (i32.load offset=4
                    (local.get $0)
                   )
                   (i32.const -8)
                  )
                  (local.get $4)
                 )
                )
                (local.get $2)
               )
               (local.set $1
                (if (result i32)
                 (local.get $16)
                 (block (result i32)
                  (local.set $2
                   (local.get $16)
                  )
                  (local.get $0)
                 )
                 (block
                  (local.set $2
                   (i32.const 0)
                  )
                  (local.set $1
                   (local.get $0)
                  )
                  (br $__rjti$1)
                 )
                )
               )
              )
              (local.set $5
               (select
                (local.get $5)
                (local.tee $5
                 (i32.load offset=20
                  (local.get $0)
                 )
                )
                (i32.or
                 (i32.eqz
                  (local.get $5)
                 )
                 (i32.eq
                  (local.get $5)
                  (local.tee $0
                   (i32.load
                    (i32.add
                     (i32.add
                      (local.get $0)
                      (i32.const 16)
                     )
                     (i32.shl
                      (i32.shr_u
                       (local.get $12)
                       (i32.const 31)
                      )
                      (i32.const 2)
                     )
                    )
                   )
                  )
                 )
                )
               )
              )
              (local.set $12
               (i32.shl
                (local.get $12)
                (i32.const 1)
               )
              )
              (br_if $while-in15
               (local.get $0)
              )
              (local.get $1)
             )
            )
            (i32.const 0)
           )
          )
          (br_if $__rjti$1
           (local.tee $1
            (if (result i32)
             (i32.or
              (local.get $0)
              (local.get $5)
             )
             (local.get $5)
             (block (result i32)
              (drop
               (br_if $do-once
                (local.get $4)
                (i32.eqz
                 (local.tee $1
                  (i32.and
                   (local.get $6)
                   (i32.or
                    (local.tee $0
                     (i32.shl
                      (i32.const 2)
                      (local.get $18)
                     )
                    )
                    (i32.sub
                     (i32.const 0)
                     (local.get $0)
                    )
                   )
                  )
                 )
                )
               )
              )
              (local.set $0
               (i32.const 0)
              )
              (i32.load
               (i32.add
                (i32.shl
                 (i32.add
                  (i32.or
                   (i32.or
                    (i32.or
                     (i32.or
                      (local.tee $5
                       (i32.and
                        (i32.shr_u
                         (local.tee $1
                          (i32.add
                           (i32.and
                            (local.get $1)
                            (i32.sub
                             (i32.const 0)
                             (local.get $1)
                            )
                           )
                           (i32.const -1)
                          )
                         )
                         (i32.const 12)
                        )
                        (i32.const 16)
                       )
                      )
                      (local.tee $5
                       (i32.and
                        (i32.shr_u
                         (local.tee $1
                          (i32.shr_u
                           (local.get $1)
                           (local.get $5)
                          )
                         )
                         (i32.const 5)
                        )
                        (i32.const 8)
                       )
                      )
                     )
                     (local.tee $5
                      (i32.and
                       (i32.shr_u
                        (local.tee $1
                         (i32.shr_u
                          (local.get $1)
                          (local.get $5)
                         )
                        )
                        (i32.const 2)
                       )
                       (i32.const 4)
                      )
                     )
                    )
                    (local.tee $5
                     (i32.and
                      (i32.shr_u
                       (local.tee $1
                        (i32.shr_u
                         (local.get $1)
                         (local.get $5)
                        )
                       )
                       (i32.const 1)
                      )
                      (i32.const 2)
                     )
                    )
                   )
                   (local.tee $5
                    (i32.and
                     (i32.shr_u
                      (local.tee $1
                       (i32.shr_u
                        (local.get $1)
                        (local.get $5)
                       )
                      )
                      (i32.const 1)
                     )
                     (i32.const 1)
                    )
                   )
                  )
                  (i32.shr_u
                   (local.get $1)
                   (local.get $5)
                  )
                 )
                 (i32.const 2)
                )
                (i32.const 1328)
               )
              )
             )
            )
           )
          )
          (local.set $5
           (local.get $0)
          )
          (br $__rjto$1)
         )
         (local.set $5
          (local.get $0)
         )
         (local.set $0
          (local.get $2)
         )
         (local.set $2
          (loop $while-in17 (result i32)
           (local.set $12
            (i32.load offset=4
             (local.get $1)
            )
           )
           (if
            (i32.eqz
             (local.tee $2
              (i32.load offset=16
               (local.get $1)
              )
             )
            )
            (local.set $2
             (i32.load offset=20
              (local.get $1)
             )
            )
           )
           (local.set $12
            (i32.lt_u
             (local.tee $16
              (i32.sub
               (i32.and
                (local.get $12)
                (i32.const -8)
               )
               (local.get $4)
              )
             )
             (local.get $0)
            )
           )
           (local.set $0
            (select
             (local.get $16)
             (local.get $0)
             (local.get $12)
            )
           )
           (local.set $5
            (select
             (local.get $1)
             (local.get $5)
             (local.get $12)
            )
           )
           (if (result i32)
            (local.get $2)
            (block
             (local.set $1
              (local.get $2)
             )
             (br $while-in17)
            )
            (local.get $0)
           )
          )
         )
        )
        (if (result i32)
         (local.get $5)
         (if (result i32)
          (i32.lt_u
           (local.get $2)
           (i32.sub
            (i32.load
             (i32.const 1032)
            )
            (local.get $4)
           )
          )
          (block
           (if
            (i32.gt_u
             (local.tee $17
              (i32.load
               (i32.const 1040)
              )
             )
             (local.get $5)
            )
            (call $_abort)
           )
           (if
            (i32.le_u
             (local.tee $8
              (i32.add
               (local.get $4)
               (local.get $5)
              )
             )
             (local.get $5)
            )
            (call $_abort)
           )
           (local.set $15
            (i32.load offset=24
             (local.get $5)
            )
           )
           (if
            (i32.eq
             (local.get $5)
             (local.tee $0
              (i32.load offset=12
               (local.get $5)
              )
             )
            )
            (block $do-once18
             (if
              (i32.eqz
               (local.tee $0
                (i32.load
                 (local.tee $1
                  (i32.add
                   (local.get $5)
                   (i32.const 20)
                  )
                 )
                )
               )
              )
              (br_if $do-once18
               (i32.eqz
                (local.tee $0
                 (i32.load
                  (local.tee $1
                   (i32.add
                    (local.get $5)
                    (i32.const 16)
                   )
                  )
                 )
                )
               )
              )
             )
             (loop $while-in21
              (block $while-out20
               (if
                (i32.eqz
                 (local.tee $11
                  (i32.load
                   (local.tee $9
                    (i32.add
                     (local.get $0)
                     (i32.const 20)
                    )
                   )
                  )
                 )
                )
                (br_if $while-out20
                 (i32.eqz
                  (local.tee $11
                   (i32.load
                    (local.tee $9
                     (i32.add
                      (local.get $0)
                      (i32.const 16)
                     )
                    )
                   )
                  )
                 )
                )
               )
               (local.set $1
                (local.get $9)
               )
               (local.set $0
                (local.get $11)
               )
               (br $while-in21)
              )
             )
             (if
              (i32.gt_u
               (local.get $17)
               (local.get $1)
              )
              (call $_abort)
              (block
               (i32.store
                (local.get $1)
                (i32.const 0)
               )
               (local.set $7
                (local.get $0)
               )
              )
             )
            )
            (block
             (if
              (i32.gt_u
               (local.get $17)
               (local.tee $1
                (i32.load offset=8
                 (local.get $5)
                )
               )
              )
              (call $_abort)
             )
             (if
              (i32.ne
               (i32.load
                (local.tee $9
                 (i32.add
                  (local.get $1)
                  (i32.const 12)
                 )
                )
               )
               (local.get $5)
              )
              (call $_abort)
             )
             (if
              (i32.eq
               (local.get $5)
               (i32.load
                (local.tee $11
                 (i32.add
                  (local.get $0)
                  (i32.const 8)
                 )
                )
               )
              )
              (block
               (i32.store
                (local.get $9)
                (local.get $0)
               )
               (i32.store
                (local.get $11)
                (local.get $1)
               )
               (local.set $7
                (local.get $0)
               )
              )
              (call $_abort)
             )
            )
           )
           (if
            (local.get $15)
            (block $label$break$L176
             (if
              (i32.eq
               (local.get $5)
               (i32.load
                (local.tee $1
                 (i32.add
                  (i32.shl
                   (local.tee $0
                    (i32.load offset=28
                     (local.get $5)
                    )
                   )
                   (i32.const 2)
                  )
                  (i32.const 1328)
                 )
                )
               )
              )
              (block
               (i32.store
                (local.get $1)
                (local.get $7)
               )
               (if
                (i32.eqz
                 (local.get $7)
                )
                (block
                 (i32.store
                  (i32.const 1028)
                  (local.tee $3
                   (i32.and
                    (local.get $6)
                    (i32.xor
                     (i32.shl
                      (i32.const 1)
                      (local.get $0)
                     )
                     (i32.const -1)
                    )
                   )
                  )
                 )
                 (br $label$break$L176)
                )
               )
              )
              (if
               (i32.gt_u
                (i32.load
                 (i32.const 1040)
                )
                (local.get $15)
               )
               (call $_abort)
               (block
                (i32.store
                 (select
                  (local.tee $0
                   (i32.add
                    (local.get $15)
                    (i32.const 16)
                   )
                  )
                  (i32.add
                   (local.get $15)
                   (i32.const 20)
                  )
                  (i32.eq
                   (local.get $5)
                   (i32.load
                    (local.get $0)
                   )
                  )
                 )
                 (local.get $7)
                )
                (if
                 (i32.eqz
                  (local.get $7)
                 )
                 (block
                  (local.set $3
                   (local.get $6)
                  )
                  (br $label$break$L176)
                 )
                )
               )
              )
             )
             (if
              (i32.gt_u
               (local.tee $1
                (i32.load
                 (i32.const 1040)
                )
               )
               (local.get $7)
              )
              (call $_abort)
             )
             (i32.store offset=24
              (local.get $7)
              (local.get $15)
             )
             (if
              (local.tee $0
               (i32.load offset=16
                (local.get $5)
               )
              )
              (if
               (i32.gt_u
                (local.get $1)
                (local.get $0)
               )
               (call $_abort)
               (block
                (i32.store offset=16
                 (local.get $7)
                 (local.get $0)
                )
                (i32.store offset=24
                 (local.get $0)
                 (local.get $7)
                )
               )
              )
             )
             (if
              (local.tee $0
               (i32.load offset=20
                (local.get $5)
               )
              )
              (if
               (i32.gt_u
                (i32.load
                 (i32.const 1040)
                )
                (local.get $0)
               )
               (call $_abort)
               (block
                (i32.store offset=20
                 (local.get $7)
                 (local.get $0)
                )
                (i32.store offset=24
                 (local.get $0)
                 (local.get $7)
                )
                (local.set $3
                 (local.get $6)
                )
               )
              )
              (local.set $3
               (local.get $6)
              )
             )
            )
            (local.set $3
             (local.get $6)
            )
           )
           (if
            (i32.lt_u
             (local.get $2)
             (i32.const 16)
            )
            (block
             (i32.store offset=4
              (local.get $5)
              (i32.or
               (local.tee $0
                (i32.add
                 (local.get $2)
                 (local.get $4)
                )
               )
               (i32.const 3)
              )
             )
             (i32.store
              (local.tee $0
               (i32.add
                (i32.add
                 (local.get $0)
                 (local.get $5)
                )
                (i32.const 4)
               )
              )
              (i32.or
               (i32.load
                (local.get $0)
               )
               (i32.const 1)
              )
             )
            )
            (block $label$break$L200
             (i32.store offset=4
              (local.get $5)
              (i32.or
               (local.get $4)
               (i32.const 3)
              )
             )
             (i32.store offset=4
              (local.get $8)
              (i32.or
               (local.get $2)
               (i32.const 1)
              )
             )
             (i32.store
              (i32.add
               (local.get $2)
               (local.get $8)
              )
              (local.get $2)
             )
             (local.set $1
              (i32.shr_u
               (local.get $2)
               (i32.const 3)
              )
             )
             (if
              (i32.lt_u
               (local.get $2)
               (i32.const 256)
              )
              (block
               (local.set $0
                (i32.add
                 (i32.shl
                  (local.get $1)
                  (i32.const 3)
                 )
                 (i32.const 1064)
                )
               )
               (if
                (i32.and
                 (local.tee $3
                  (i32.load
                   (i32.const 1024)
                  )
                 )
                 (local.tee $1
                  (i32.shl
                   (i32.const 1)
                   (local.get $1)
                  )
                 )
                )
                (if
                 (i32.gt_u
                  (i32.load
                   (i32.const 1040)
                  )
                  (local.tee $1
                   (i32.load
                    (local.tee $3
                     (i32.add
                      (local.get $0)
                      (i32.const 8)
                     )
                    )
                   )
                  )
                 )
                 (call $_abort)
                 (block
                  (local.set $13
                   (local.get $1)
                  )
                  (local.set $19
                   (local.get $3)
                  )
                 )
                )
                (block
                 (i32.store
                  (i32.const 1024)
                  (i32.or
                   (local.get $1)
                   (local.get $3)
                  )
                 )
                 (local.set $13
                  (local.get $0)
                 )
                 (local.set $19
                  (i32.add
                   (local.get $0)
                   (i32.const 8)
                  )
                 )
                )
               )
               (i32.store
                (local.get $19)
                (local.get $8)
               )
               (i32.store offset=12
                (local.get $13)
                (local.get $8)
               )
               (i32.store offset=8
                (local.get $8)
                (local.get $13)
               )
               (i32.store offset=12
                (local.get $8)
                (local.get $0)
               )
               (br $label$break$L200)
              )
             )
             (local.set $0
              (i32.add
               (i32.shl
                (local.tee $1
                 (if (result i32)
                  (local.tee $0
                   (i32.shr_u
                    (local.get $2)
                    (i32.const 8)
                   )
                  )
                  (if (result i32)
                   (i32.gt_u
                    (local.get $2)
                    (i32.const 16777215)
                   )
                   (i32.const 31)
                   (block (result i32)
                    (local.set $0
                     (i32.and
                      (i32.shr_u
                       (i32.add
                        (local.tee $4
                         (i32.shl
                          (local.get $0)
                          (local.tee $1
                           (i32.and
                            (i32.shr_u
                             (i32.add
                              (local.get $0)
                              (i32.const 1048320)
                             )
                             (i32.const 16)
                            )
                            (i32.const 8)
                           )
                          )
                         )
                        )
                        (i32.const 520192)
                       )
                       (i32.const 16)
                      )
                      (i32.const 4)
                     )
                    )
                    (i32.or
                     (i32.shl
                      (local.tee $0
                       (i32.add
                        (i32.sub
                         (i32.const 14)
                         (i32.or
                          (i32.or
                           (local.get $0)
                           (local.get $1)
                          )
                          (local.tee $1
                           (i32.and
                            (i32.shr_u
                             (i32.add
                              (local.tee $0
                               (i32.shl
                                (local.get $4)
                                (local.get $0)
                               )
                              )
                              (i32.const 245760)
                             )
                             (i32.const 16)
                            )
                            (i32.const 2)
                           )
                          )
                         )
                        )
                        (i32.shr_u
                         (i32.shl
                          (local.get $0)
                          (local.get $1)
                         )
                         (i32.const 15)
                        )
                       )
                      )
                      (i32.const 1)
                     )
                     (i32.and
                      (i32.shr_u
                       (local.get $2)
                       (i32.add
                        (local.get $0)
                        (i32.const 7)
                       )
                      )
                      (i32.const 1)
                     )
                    )
                   )
                  )
                  (i32.const 0)
                 )
                )
                (i32.const 2)
               )
               (i32.const 1328)
              )
             )
             (i32.store offset=28
              (local.get $8)
              (local.get $1)
             )
             (i32.store offset=4
              (local.tee $4
               (i32.add
                (local.get $8)
                (i32.const 16)
               )
              )
              (i32.const 0)
             )
             (i32.store
              (local.get $4)
              (i32.const 0)
             )
             (if
              (i32.eqz
               (i32.and
                (local.tee $4
                 (i32.shl
                  (i32.const 1)
                  (local.get $1)
                 )
                )
                (local.get $3)
               )
              )
              (block
               (i32.store
                (i32.const 1028)
                (i32.or
                 (local.get $3)
                 (local.get $4)
                )
               )
               (i32.store
                (local.get $0)
                (local.get $8)
               )
               (i32.store offset=24
                (local.get $8)
                (local.get $0)
               )
               (i32.store offset=12
                (local.get $8)
                (local.get $8)
               )
               (i32.store offset=8
                (local.get $8)
                (local.get $8)
               )
               (br $label$break$L200)
              )
             )
             (if
              (i32.eq
               (local.get $2)
               (i32.and
                (i32.load offset=4
                 (local.tee $0
                  (i32.load
                   (local.get $0)
                  )
                 )
                )
                (i32.const -8)
               )
              )
              (local.set $10
               (local.get $0)
              )
              (block $label$break$L218
               (local.set $1
                (i32.shl
                 (local.get $2)
                 (select
                  (i32.const 0)
                  (i32.sub
                   (i32.const 25)
                   (i32.shr_u
                    (local.get $1)
                    (i32.const 1)
                   )
                  )
                  (i32.eq
                   (local.get $1)
                   (i32.const 31)
                  )
                 )
                )
               )
               (loop $while-in30
                (if
                 (local.tee $3
                  (i32.load
                   (local.tee $4
                    (i32.add
                     (i32.add
                      (local.get $0)
                      (i32.const 16)
                     )
                     (i32.shl
                      (i32.shr_u
                       (local.get $1)
                       (i32.const 31)
                      )
                      (i32.const 2)
                     )
                    )
                   )
                  )
                 )
                 (block
                  (local.set $1
                   (i32.shl
                    (local.get $1)
                    (i32.const 1)
                   )
                  )
                  (if
                   (i32.eq
                    (local.get $2)
                    (i32.and
                     (i32.load offset=4
                      (local.get $3)
                     )
                     (i32.const -8)
                    )
                   )
                   (block
                    (local.set $10
                     (local.get $3)
                    )
                    (br $label$break$L218)
                   )
                   (block
                    (local.set $0
                     (local.get $3)
                    )
                    (br $while-in30)
                   )
                  )
                 )
                )
               )
               (if
                (i32.gt_u
                 (i32.load
                  (i32.const 1040)
                 )
                 (local.get $4)
                )
                (call $_abort)
                (block
                 (i32.store
                  (local.get $4)
                  (local.get $8)
                 )
                 (i32.store offset=24
                  (local.get $8)
                  (local.get $0)
                 )
                 (i32.store offset=12
                  (local.get $8)
                  (local.get $8)
                 )
                 (i32.store offset=8
                  (local.get $8)
                  (local.get $8)
                 )
                 (br $label$break$L200)
                )
               )
              )
             )
             (if
              (i32.and
               (i32.le_u
                (local.tee $0
                 (i32.load
                  (i32.const 1040)
                 )
                )
                (local.get $10)
               )
               (i32.le_u
                (local.get $0)
                (local.tee $0
                 (i32.load
                  (local.tee $3
                   (i32.add
                    (local.get $10)
                    (i32.const 8)
                   )
                  )
                 )
                )
               )
              )
              (block
               (i32.store offset=12
                (local.get $0)
                (local.get $8)
               )
               (i32.store
                (local.get $3)
                (local.get $8)
               )
               (i32.store offset=8
                (local.get $8)
                (local.get $0)
               )
               (i32.store offset=12
                (local.get $8)
                (local.get $10)
               )
               (i32.store offset=24
                (local.get $8)
                (i32.const 0)
               )
              )
              (call $_abort)
             )
            )
           )
           (global.set $STACKTOP
            (local.get $14)
           )
           (return
            (i32.add
             (local.get $5)
             (i32.const 8)
            )
           )
          )
          (local.get $4)
         )
         (local.get $4)
        )
       )
       (local.get $4)
      )
     )
    )
   )
  )
  (block $folding-inner1
   (if
    (i32.ge_u
     (local.tee $1
      (i32.load
       (i32.const 1032)
      )
     )
     (local.get $3)
    )
    (block
     (local.set $0
      (i32.load
       (i32.const 1044)
      )
     )
     (if
      (i32.gt_u
       (local.tee $2
        (i32.sub
         (local.get $1)
         (local.get $3)
        )
       )
       (i32.const 15)
      )
      (block
       (i32.store
        (i32.const 1044)
        (local.tee $4
         (i32.add
          (local.get $0)
          (local.get $3)
         )
        )
       )
       (i32.store
        (i32.const 1032)
        (local.get $2)
       )
       (i32.store offset=4
        (local.get $4)
        (i32.or
         (local.get $2)
         (i32.const 1)
        )
       )
       (i32.store
        (i32.add
         (local.get $0)
         (local.get $1)
        )
        (local.get $2)
       )
       (i32.store offset=4
        (local.get $0)
        (i32.or
         (local.get $3)
         (i32.const 3)
        )
       )
      )
      (block
       (i32.store
        (i32.const 1032)
        (i32.const 0)
       )
       (i32.store
        (i32.const 1044)
        (i32.const 0)
       )
       (i32.store offset=4
        (local.get $0)
        (i32.or
         (local.get $1)
         (i32.const 3)
        )
       )
       (i32.store
        (local.tee $3
         (i32.add
          (i32.add
           (local.get $0)
           (local.get $1)
          )
          (i32.const 4)
         )
        )
        (i32.or
         (i32.load
          (local.get $3)
         )
         (i32.const 1)
        )
       )
      )
     )
     (br $folding-inner1)
    )
   )
   (block $folding-inner0
    (if
     (i32.gt_u
      (local.tee $1
       (i32.load
        (i32.const 1036)
       )
      )
      (local.get $3)
     )
     (block
      (i32.store
       (i32.const 1036)
       (local.tee $1
        (i32.sub
         (local.get $1)
         (local.get $3)
        )
       )
      )
      (br $folding-inner0)
     )
    )
    (if
     (i32.le_u
      (local.tee $4
       (i32.and
        (local.tee $5
         (i32.add
          (local.tee $6
           (i32.add
            (local.get $3)
            (i32.const 47)
           )
          )
          (local.tee $0
           (if (result i32)
            (i32.load
             (i32.const 1496)
            )
            (i32.load
             (i32.const 1504)
            )
            (block (result i32)
             (i32.store
              (i32.const 1504)
              (i32.const 4096)
             )
             (i32.store
              (i32.const 1500)
              (i32.const 4096)
             )
             (i32.store
              (i32.const 1508)
              (i32.const -1)
             )
             (i32.store
              (i32.const 1512)
              (i32.const -1)
             )
             (i32.store
              (i32.const 1516)
              (i32.const 0)
             )
             (i32.store
              (i32.const 1468)
              (i32.const 0)
             )
             (i32.store
              (i32.const 1496)
              (i32.xor
               (i32.and
                (local.get $14)
                (i32.const -16)
               )
               (i32.const 1431655768)
              )
             )
             (i32.const 4096)
            )
           )
          )
         )
        )
        (local.tee $7
         (i32.sub
          (i32.const 0)
          (local.get $0)
         )
        )
       )
      )
      (local.get $3)
     )
     (block
      (global.set $STACKTOP
       (local.get $14)
      )
      (return
       (i32.const 0)
      )
     )
    )
    (if
     (local.tee $0
      (i32.load
       (i32.const 1464)
      )
     )
     (if
      (i32.or
       (i32.le_u
        (local.tee $10
         (i32.add
          (local.get $4)
          (local.tee $2
           (i32.load
            (i32.const 1456)
           )
          )
         )
        )
        (local.get $2)
       )
       (i32.gt_u
        (local.get $10)
        (local.get $0)
       )
      )
      (block
       (global.set $STACKTOP
        (local.get $14)
       )
       (return
        (i32.const 0)
       )
      )
     )
    )
    (local.set $10
     (i32.add
      (local.get $3)
      (i32.const 48)
     )
    )
    (block $__rjto$7
     (block $__rjti$7
      (if
       (i32.and
        (i32.load
         (i32.const 1468)
        )
        (i32.const 4)
       )
       (local.set $1
        (i32.const 0)
       )
       (block
        (block $do-once37
         (block $__rjti$3
          (block $__rjti$2
           (br_if $__rjti$2
            (i32.eqz
             (local.tee $0
              (i32.load
               (i32.const 1048)
              )
             )
            )
           )
           (local.set $2
            (i32.const 1472)
           )
           (loop $while-in34
            (block $while-out33
             (if
              (i32.le_u
               (local.tee $13
                (i32.load
                 (local.get $2)
                )
               )
               (local.get $0)
              )
              (br_if $while-out33
               (i32.gt_u
                (i32.add
                 (local.get $13)
                 (i32.load offset=4
                  (local.get $2)
                 )
                )
                (local.get $0)
               )
              )
             )
             (br_if $while-in34
              (local.tee $2
               (i32.load offset=8
                (local.get $2)
               )
              )
             )
             (br $__rjti$2)
            )
           )
           (if
            (i32.lt_u
             (local.tee $1
              (i32.and
               (local.get $7)
               (i32.sub
                (local.get $5)
                (local.get $1)
               )
              )
             )
             (i32.const 2147483647)
            )
            (block
             (local.set $5
              (i32.add
               (local.get $2)
               (i32.const 4)
              )
             )
             (br_if $__rjti$3
              (i32.ne
               (local.tee $0
                (call $_sbrk
                 (local.get $1)
                )
               )
               (i32.add
                (i32.load
                 (local.get $2)
                )
                (i32.load
                 (local.get $5)
                )
               )
              )
             )
             (br_if $__rjti$7
              (i32.ne
               (local.get $0)
               (i32.const -1)
              )
             )
            )
            (local.set $1
             (i32.const 0)
            )
           )
           (br $do-once37)
          )
          (local.set $1
           (if (result i32)
            (i32.eq
             (local.tee $0
              (call $_sbrk
               (i32.const 0)
              )
             )
             (i32.const -1)
            )
            (i32.const 0)
            (block (result i32)
             (local.set $2
              (i32.add
               (local.tee $5
                (i32.load
                 (i32.const 1456)
                )
               )
               (local.tee $1
                (i32.add
                 (local.get $4)
                 (select
                  (i32.sub
                   (i32.and
                    (i32.add
                     (local.get $0)
                     (local.tee $2
                      (i32.add
                       (local.tee $1
                        (i32.load
                         (i32.const 1500)
                        )
                       )
                       (i32.const -1)
                      )
                     )
                    )
                    (i32.sub
                     (i32.const 0)
                     (local.get $1)
                    )
                   )
                   (local.get $0)
                  )
                  (i32.const 0)
                  (i32.and
                   (local.get $0)
                   (local.get $2)
                  )
                 )
                )
               )
              )
             )
             (if (result i32)
              (i32.and
               (i32.lt_u
                (local.get $1)
                (i32.const 2147483647)
               )
               (i32.gt_u
                (local.get $1)
                (local.get $3)
               )
              )
              (block
               (if
                (local.tee $7
                 (i32.load
                  (i32.const 1464)
                 )
                )
                (if
                 (i32.or
                  (i32.le_u
                   (local.get $2)
                   (local.get $5)
                  )
                  (i32.gt_u
                   (local.get $2)
                   (local.get $7)
                  )
                 )
                 (block
                  (local.set $1
                   (i32.const 0)
                  )
                  (br $do-once37)
                 )
                )
               )
               (br_if $__rjti$7
                (i32.eq
                 (local.get $0)
                 (local.tee $2
                  (call $_sbrk
                   (local.get $1)
                  )
                 )
                )
               )
               (local.set $0
                (local.get $2)
               )
               (br $__rjti$3)
              )
              (i32.const 0)
             )
            )
           )
          )
          (br $do-once37)
         )
         (if
          (i32.eqz
           (i32.and
            (i32.and
             (i32.ne
              (local.get $0)
              (i32.const -1)
             )
             (i32.lt_u
              (local.get $1)
              (i32.const 2147483647)
             )
            )
            (i32.gt_u
             (local.get $10)
             (local.get $1)
            )
           )
          )
          (if
           (i32.eq
            (local.get $0)
            (i32.const -1)
           )
           (block
            (local.set $1
             (i32.const 0)
            )
            (br $do-once37)
           )
           (br $__rjti$7)
          )
         )
         (br_if $__rjti$7
          (i32.ge_u
           (local.tee $2
            (i32.and
             (i32.add
              (local.tee $2
               (i32.load
                (i32.const 1504)
               )
              )
              (i32.sub
               (local.get $6)
               (local.get $1)
              )
             )
             (i32.sub
              (i32.const 0)
              (local.get $2)
             )
            )
           )
           (i32.const 2147483647)
          )
         )
         (local.set $6
          (i32.sub
           (i32.const 0)
           (local.get $1)
          )
         )
         (local.set $1
          (if (result i32)
           (i32.eq
            (call $_sbrk
             (local.get $2)
            )
            (i32.const -1)
           )
           (block (result i32)
            (drop
             (call $_sbrk
              (local.get $6)
             )
            )
            (i32.const 0)
           )
           (block
            (local.set $1
             (i32.add
              (local.get $1)
              (local.get $2)
             )
            )
            (br $__rjti$7)
           )
          )
         )
        )
        (i32.store
         (i32.const 1468)
         (i32.or
          (i32.load
           (i32.const 1468)
          )
          (i32.const 4)
         )
        )
       )
      )
      (if
       (i32.lt_u
        (local.get $4)
        (i32.const 2147483647)
       )
       (block
        (local.set $0
         (call $_sbrk
          (local.get $4)
         )
        )
        (local.set $4
         (i32.gt_u
          (local.tee $6
           (i32.sub
            (local.tee $2
             (call $_sbrk
              (i32.const 0)
             )
            )
            (local.get $0)
           )
          )
          (i32.add
           (local.get $3)
           (i32.const 40)
          )
         )
        )
        (local.set $1
         (select
          (local.get $6)
          (local.get $1)
          (local.get $4)
         )
        )
        (br_if $__rjti$7
         (i32.eqz
          (i32.or
           (i32.or
            (i32.xor
             (local.get $4)
             (i32.const 1)
            )
            (i32.eq
             (local.get $0)
             (i32.const -1)
            )
           )
           (i32.xor
            (i32.and
             (i32.and
              (i32.ne
               (local.get $0)
               (i32.const -1)
              )
              (i32.ne
               (local.get $2)
               (i32.const -1)
              )
             )
             (i32.lt_u
              (local.get $0)
              (local.get $2)
             )
            )
            (i32.const 1)
           )
          )
         )
        )
       )
      )
      (br $__rjto$7)
     )
     (i32.store
      (i32.const 1456)
      (local.tee $2
       (i32.add
        (local.get $1)
        (i32.load
         (i32.const 1456)
        )
       )
      )
     )
     (if
      (i32.gt_u
       (local.get $2)
       (i32.load
        (i32.const 1460)
       )
      )
      (i32.store
       (i32.const 1460)
       (local.get $2)
      )
     )
     (if
      (local.tee $6
       (i32.load
        (i32.const 1048)
       )
      )
      (block $label$break$L294
       (local.set $2
        (i32.const 1472)
       )
       (block $__rjto$4
        (block $__rjti$4
         (loop $while-in41
          (br_if $__rjti$4
           (i32.eq
            (local.get $0)
            (i32.add
             (local.tee $4
              (i32.load
               (local.get $2)
              )
             )
             (local.tee $5
              (i32.load offset=4
               (local.get $2)
              )
             )
            )
           )
          )
          (br_if $while-in41
           (local.tee $2
            (i32.load offset=8
             (local.get $2)
            )
           )
          )
         )
         (br $__rjto$4)
        )
        (local.set $7
         (i32.add
          (local.get $2)
          (i32.const 4)
         )
        )
        (if
         (i32.eqz
          (i32.and
           (i32.load offset=12
            (local.get $2)
           )
           (i32.const 8)
          )
         )
         (if
          (i32.and
           (i32.le_u
            (local.get $4)
            (local.get $6)
           )
           (i32.gt_u
            (local.get $0)
            (local.get $6)
           )
          )
          (block
           (i32.store
            (local.get $7)
            (i32.add
             (local.get $1)
             (local.get $5)
            )
           )
           (local.set $0
            (i32.add
             (local.get $6)
             (local.tee $2
              (select
               (i32.and
                (i32.sub
                 (i32.const 0)
                 (local.tee $0
                  (i32.add
                   (local.get $6)
                   (i32.const 8)
                  )
                 )
                )
                (i32.const 7)
               )
               (i32.const 0)
               (i32.and
                (local.get $0)
                (i32.const 7)
               )
              )
             )
            )
           )
           (local.set $1
            (i32.sub
             (local.tee $4
              (i32.add
               (local.get $1)
               (i32.load
                (i32.const 1036)
               )
              )
             )
             (local.get $2)
            )
           )
           (i32.store
            (i32.const 1048)
            (local.get $0)
           )
           (i32.store
            (i32.const 1036)
            (local.get $1)
           )
           (i32.store offset=4
            (local.get $0)
            (i32.or
             (local.get $1)
             (i32.const 1)
            )
           )
           (i32.store offset=4
            (i32.add
             (local.get $4)
             (local.get $6)
            )
            (i32.const 40)
           )
           (i32.store
            (i32.const 1052)
            (i32.load
             (i32.const 1512)
            )
           )
           (br $label$break$L294)
          )
         )
        )
       )
       (if
        (i32.lt_u
         (local.get $0)
         (local.tee $2
          (i32.load
           (i32.const 1040)
          )
         )
        )
        (block
         (i32.store
          (i32.const 1040)
          (local.get $0)
         )
         (local.set $2
          (local.get $0)
         )
        )
       )
       (local.set $5
        (i32.add
         (local.get $0)
         (local.get $1)
        )
       )
       (local.set $4
        (i32.const 1472)
       )
       (block $__rjto$5
        (block $__rjti$5
         (loop $while-in43
          (br_if $__rjti$5
           (i32.eq
            (local.get $5)
            (i32.load
             (local.get $4)
            )
           )
          )
          (br_if $while-in43
           (local.tee $4
            (i32.load offset=8
             (local.get $4)
            )
           )
          )
         )
         (br $__rjto$5)
        )
        (if
         (i32.eqz
          (i32.and
           (i32.load offset=12
            (local.get $4)
           )
           (i32.const 8)
          )
         )
         (block
          (i32.store
           (local.get $4)
           (local.get $0)
          )
          (i32.store
           (local.tee $4
            (i32.add
             (local.get $4)
             (i32.const 4)
            )
           )
           (i32.add
            (local.get $1)
            (i32.load
             (local.get $4)
            )
           )
          )
          (local.set $7
           (i32.add
            (local.get $3)
            (local.tee $8
             (i32.add
              (local.get $0)
              (select
               (i32.and
                (i32.sub
                 (i32.const 0)
                 (local.tee $0
                  (i32.add
                   (local.get $0)
                   (i32.const 8)
                  )
                 )
                )
                (i32.const 7)
               )
               (i32.const 0)
               (i32.and
                (local.get $0)
                (i32.const 7)
               )
              )
             )
            )
           )
          )
          (local.set $4
           (i32.sub
            (i32.sub
             (local.tee $1
              (i32.add
               (local.get $5)
               (select
                (i32.and
                 (i32.sub
                  (i32.const 0)
                  (local.tee $0
                   (i32.add
                    (local.get $5)
                    (i32.const 8)
                   )
                  )
                 )
                 (i32.const 7)
                )
                (i32.const 0)
                (i32.and
                 (local.get $0)
                 (i32.const 7)
                )
               )
              )
             )
             (local.get $8)
            )
            (local.get $3)
           )
          )
          (i32.store offset=4
           (local.get $8)
           (i32.or
            (local.get $3)
            (i32.const 3)
           )
          )
          (if
           (i32.eq
            (local.get $1)
            (local.get $6)
           )
           (block
            (i32.store
             (i32.const 1036)
             (local.tee $0
              (i32.add
               (local.get $4)
               (i32.load
                (i32.const 1036)
               )
              )
             )
            )
            (i32.store
             (i32.const 1048)
             (local.get $7)
            )
            (i32.store offset=4
             (local.get $7)
             (i32.or
              (local.get $0)
              (i32.const 1)
             )
            )
           )
           (block $label$break$L317
            (if
             (i32.eq
              (local.get $1)
              (i32.load
               (i32.const 1044)
              )
             )
             (block
              (i32.store
               (i32.const 1032)
               (local.tee $0
                (i32.add
                 (local.get $4)
                 (i32.load
                  (i32.const 1032)
                 )
                )
               )
              )
              (i32.store
               (i32.const 1044)
               (local.get $7)
              )
              (i32.store offset=4
               (local.get $7)
               (i32.or
                (local.get $0)
                (i32.const 1)
               )
              )
              (i32.store
               (i32.add
                (local.get $0)
                (local.get $7)
               )
               (local.get $0)
              )
              (br $label$break$L317)
             )
            )
            (local.set $2
             (if (result i32)
              (i32.eq
               (i32.and
                (local.tee $0
                 (i32.load offset=4
                  (local.get $1)
                 )
                )
                (i32.const 3)
               )
               (i32.const 1)
              )
              (block (result i32)
               (local.set $13
                (i32.and
                 (local.get $0)
                 (i32.const -8)
                )
               )
               (local.set $5
                (i32.shr_u
                 (local.get $0)
                 (i32.const 3)
                )
               )
               (block $label$break$L325
                (if
                 (i32.lt_u
                  (local.get $0)
                  (i32.const 256)
                 )
                 (block
                  (local.set $3
                   (i32.load offset=12
                    (local.get $1)
                   )
                  )
                  (if
                   (i32.ne
                    (local.tee $6
                     (i32.load offset=8
                      (local.get $1)
                     )
                    )
                    (local.tee $0
                     (i32.add
                      (i32.shl
                       (local.get $5)
                       (i32.const 3)
                      )
                      (i32.const 1064)
                     )
                    )
                   )
                   (block $do-once46
                    (if
                     (i32.gt_u
                      (local.get $2)
                      (local.get $6)
                     )
                     (call $_abort)
                    )
                    (br_if $do-once46
                     (i32.eq
                      (local.get $1)
                      (i32.load offset=12
                       (local.get $6)
                      )
                     )
                    )
                    (call $_abort)
                   )
                  )
                  (if
                   (i32.eq
                    (local.get $3)
                    (local.get $6)
                   )
                   (block
                    (i32.store
                     (i32.const 1024)
                     (i32.and
                      (i32.load
                       (i32.const 1024)
                      )
                      (i32.xor
                       (i32.shl
                        (i32.const 1)
                        (local.get $5)
                       )
                       (i32.const -1)
                      )
                     )
                    )
                    (br $label$break$L325)
                   )
                  )
                  (if
                   (i32.eq
                    (local.get $0)
                    (local.get $3)
                   )
                   (local.set $20
                    (i32.add
                     (local.get $3)
                     (i32.const 8)
                    )
                   )
                   (block $do-once48
                    (if
                     (i32.gt_u
                      (local.get $2)
                      (local.get $3)
                     )
                     (call $_abort)
                    )
                    (if
                     (i32.eq
                      (local.get $1)
                      (i32.load
                       (local.tee $0
                        (i32.add
                         (local.get $3)
                         (i32.const 8)
                        )
                       )
                      )
                     )
                     (block
                      (local.set $20
                       (local.get $0)
                      )
                      (br $do-once48)
                     )
                    )
                    (call $_abort)
                   )
                  )
                  (i32.store offset=12
                   (local.get $6)
                   (local.get $3)
                  )
                  (i32.store
                   (local.get $20)
                   (local.get $6)
                  )
                 )
                 (block
                  (local.set $10
                   (i32.load offset=24
                    (local.get $1)
                   )
                  )
                  (if
                   (i32.eq
                    (local.get $1)
                    (local.tee $0
                     (i32.load offset=12
                      (local.get $1)
                     )
                    )
                   )
                   (block $do-once50
                    (if
                     (local.tee $0
                      (i32.load
                       (local.tee $6
                        (i32.add
                         (local.tee $3
                          (i32.add
                           (local.get $1)
                           (i32.const 16)
                          )
                         )
                         (i32.const 4)
                        )
                       )
                      )
                     )
                     (local.set $3
                      (local.get $6)
                     )
                     (br_if $do-once50
                      (i32.eqz
                       (local.tee $0
                        (i32.load
                         (local.get $3)
                        )
                       )
                      )
                     )
                    )
                    (loop $while-in53
                     (block $while-out52
                      (if
                       (i32.eqz
                        (local.tee $5
                         (i32.load
                          (local.tee $6
                           (i32.add
                            (local.get $0)
                            (i32.const 20)
                           )
                          )
                         )
                        )
                       )
                       (br_if $while-out52
                        (i32.eqz
                         (local.tee $5
                          (i32.load
                           (local.tee $6
                            (i32.add
                             (local.get $0)
                             (i32.const 16)
                            )
                           )
                          )
                         )
                        )
                       )
                      )
                      (local.set $3
                       (local.get $6)
                      )
                      (local.set $0
                       (local.get $5)
                      )
                      (br $while-in53)
                     )
                    )
                    (if
                     (i32.gt_u
                      (local.get $2)
                      (local.get $3)
                     )
                     (call $_abort)
                     (block
                      (i32.store
                       (local.get $3)
                       (i32.const 0)
                      )
                      (local.set $9
                       (local.get $0)
                      )
                     )
                    )
                   )
                   (block
                    (if
                     (i32.gt_u
                      (local.get $2)
                      (local.tee $3
                       (i32.load offset=8
                        (local.get $1)
                       )
                      )
                     )
                     (call $_abort)
                    )
                    (if
                     (i32.ne
                      (i32.load
                       (local.tee $2
                        (i32.add
                         (local.get $3)
                         (i32.const 12)
                        )
                       )
                      )
                      (local.get $1)
                     )
                     (call $_abort)
                    )
                    (if
                     (i32.eq
                      (local.get $1)
                      (i32.load
                       (local.tee $6
                        (i32.add
                         (local.get $0)
                         (i32.const 8)
                        )
                       )
                      )
                     )
                     (block
                      (i32.store
                       (local.get $2)
                       (local.get $0)
                      )
                      (i32.store
                       (local.get $6)
                       (local.get $3)
                      )
                      (local.set $9
                       (local.get $0)
                      )
                     )
                     (call $_abort)
                    )
                   )
                  )
                  (br_if $label$break$L325
                   (i32.eqz
                    (local.get $10)
                   )
                  )
                  (if
                   (i32.eq
                    (local.get $1)
                    (i32.load
                     (local.tee $3
                      (i32.add
                       (i32.shl
                        (local.tee $0
                         (i32.load offset=28
                          (local.get $1)
                         )
                        )
                        (i32.const 2)
                       )
                       (i32.const 1328)
                      )
                     )
                    )
                   )
                   (block $do-once54
                    (i32.store
                     (local.get $3)
                     (local.get $9)
                    )
                    (br_if $do-once54
                     (local.get $9)
                    )
                    (i32.store
                     (i32.const 1028)
                     (i32.and
                      (i32.load
                       (i32.const 1028)
                      )
                      (i32.xor
                       (i32.shl
                        (i32.const 1)
                        (local.get $0)
                       )
                       (i32.const -1)
                      )
                     )
                    )
                    (br $label$break$L325)
                   )
                   (if
                    (i32.gt_u
                     (i32.load
                      (i32.const 1040)
                     )
                     (local.get $10)
                    )
                    (call $_abort)
                    (block
                     (i32.store
                      (select
                       (local.tee $0
                        (i32.add
                         (local.get $10)
                         (i32.const 16)
                        )
                       )
                       (i32.add
                        (local.get $10)
                        (i32.const 20)
                       )
                       (i32.eq
                        (local.get $1)
                        (i32.load
                         (local.get $0)
                        )
                       )
                      )
                      (local.get $9)
                     )
                     (br_if $label$break$L325
                      (i32.eqz
                       (local.get $9)
                      )
                     )
                    )
                   )
                  )
                  (if
                   (i32.gt_u
                    (local.tee $3
                     (i32.load
                      (i32.const 1040)
                     )
                    )
                    (local.get $9)
                   )
                   (call $_abort)
                  )
                  (i32.store offset=24
                   (local.get $9)
                   (local.get $10)
                  )
                  (if
                   (local.tee $0
                    (i32.load
                     (local.tee $2
                      (i32.add
                       (local.get $1)
                       (i32.const 16)
                      )
                     )
                    )
                   )
                   (if
                    (i32.gt_u
                     (local.get $3)
                     (local.get $0)
                    )
                    (call $_abort)
                    (block
                     (i32.store offset=16
                      (local.get $9)
                      (local.get $0)
                     )
                     (i32.store offset=24
                      (local.get $0)
                      (local.get $9)
                     )
                    )
                   )
                  )
                  (br_if $label$break$L325
                   (i32.eqz
                    (local.tee $0
                     (i32.load offset=4
                      (local.get $2)
                     )
                    )
                   )
                  )
                  (if
                   (i32.gt_u
                    (i32.load
                     (i32.const 1040)
                    )
                    (local.get $0)
                   )
                   (call $_abort)
                   (block
                    (i32.store offset=20
                     (local.get $9)
                     (local.get $0)
                    )
                    (i32.store offset=24
                     (local.get $0)
                     (local.get $9)
                    )
                   )
                  )
                 )
                )
               )
               (local.set $1
                (i32.add
                 (local.get $1)
                 (local.get $13)
                )
               )
               (i32.add
                (local.get $4)
                (local.get $13)
               )
              )
              (local.get $4)
             )
            )
            (i32.store
             (local.tee $0
              (i32.add
               (local.get $1)
               (i32.const 4)
              )
             )
             (i32.and
              (i32.load
               (local.get $0)
              )
              (i32.const -2)
             )
            )
            (i32.store offset=4
             (local.get $7)
             (i32.or
              (local.get $2)
              (i32.const 1)
             )
            )
            (i32.store
             (i32.add
              (local.get $2)
              (local.get $7)
             )
             (local.get $2)
            )
            (local.set $3
             (i32.shr_u
              (local.get $2)
              (i32.const 3)
             )
            )
            (if
             (i32.lt_u
              (local.get $2)
              (i32.const 256)
             )
             (block
              (local.set $0
               (i32.add
                (i32.shl
                 (local.get $3)
                 (i32.const 3)
                )
                (i32.const 1064)
               )
              )
              (if
               (i32.and
                (local.tee $1
                 (i32.load
                  (i32.const 1024)
                 )
                )
                (local.tee $3
                 (i32.shl
                  (i32.const 1)
                  (local.get $3)
                 )
                )
               )
               (block $do-once58
                (if
                 (i32.le_u
                  (i32.load
                   (i32.const 1040)
                  )
                  (local.tee $1
                   (i32.load
                    (local.tee $3
                     (i32.add
                      (local.get $0)
                      (i32.const 8)
                     )
                    )
                   )
                  )
                 )
                 (block
                  (local.set $15
                   (local.get $1)
                  )
                  (local.set $21
                   (local.get $3)
                  )
                  (br $do-once58)
                 )
                )
                (call $_abort)
               )
               (block
                (i32.store
                 (i32.const 1024)
                 (i32.or
                  (local.get $1)
                  (local.get $3)
                 )
                )
                (local.set $15
                 (local.get $0)
                )
                (local.set $21
                 (i32.add
                  (local.get $0)
                  (i32.const 8)
                 )
                )
               )
              )
              (i32.store
               (local.get $21)
               (local.get $7)
              )
              (i32.store offset=12
               (local.get $15)
               (local.get $7)
              )
              (i32.store offset=8
               (local.get $7)
               (local.get $15)
              )
              (i32.store offset=12
               (local.get $7)
               (local.get $0)
              )
              (br $label$break$L317)
             )
            )
            (local.set $0
             (i32.add
              (i32.shl
               (local.tee $3
                (if (result i32)
                 (local.tee $0
                  (i32.shr_u
                   (local.get $2)
                   (i32.const 8)
                  )
                 )
                 (if (result i32)
                  (i32.gt_u
                   (local.get $2)
                   (i32.const 16777215)
                  )
                  (i32.const 31)
                  (block (result i32)
                   (local.set $0
                    (i32.and
                     (i32.shr_u
                      (i32.add
                       (local.tee $1
                        (i32.shl
                         (local.get $0)
                         (local.tee $3
                          (i32.and
                           (i32.shr_u
                            (i32.add
                             (local.get $0)
                             (i32.const 1048320)
                            )
                            (i32.const 16)
                           )
                           (i32.const 8)
                          )
                         )
                        )
                       )
                       (i32.const 520192)
                      )
                      (i32.const 16)
                     )
                     (i32.const 4)
                    )
                   )
                   (i32.or
                    (i32.shl
                     (local.tee $0
                      (i32.add
                       (i32.sub
                        (i32.const 14)
                        (i32.or
                         (i32.or
                          (local.get $0)
                          (local.get $3)
                         )
                         (local.tee $3
                          (i32.and
                           (i32.shr_u
                            (i32.add
                             (local.tee $0
                              (i32.shl
                               (local.get $1)
                               (local.get $0)
                              )
                             )
                             (i32.const 245760)
                            )
                            (i32.const 16)
                           )
                           (i32.const 2)
                          )
                         )
                        )
                       )
                       (i32.shr_u
                        (i32.shl
                         (local.get $0)
                         (local.get $3)
                        )
                        (i32.const 15)
                       )
                      )
                     )
                     (i32.const 1)
                    )
                    (i32.and
                     (i32.shr_u
                      (local.get $2)
                      (i32.add
                       (local.get $0)
                       (i32.const 7)
                      )
                     )
                     (i32.const 1)
                    )
                   )
                  )
                 )
                 (i32.const 0)
                )
               )
               (i32.const 2)
              )
              (i32.const 1328)
             )
            )
            (i32.store offset=28
             (local.get $7)
             (local.get $3)
            )
            (i32.store offset=4
             (local.tee $1
              (i32.add
               (local.get $7)
               (i32.const 16)
              )
             )
             (i32.const 0)
            )
            (i32.store
             (local.get $1)
             (i32.const 0)
            )
            (if
             (i32.eqz
              (i32.and
               (local.tee $1
                (i32.load
                 (i32.const 1028)
                )
               )
               (local.tee $4
                (i32.shl
                 (i32.const 1)
                 (local.get $3)
                )
               )
              )
             )
             (block
              (i32.store
               (i32.const 1028)
               (i32.or
                (local.get $1)
                (local.get $4)
               )
              )
              (i32.store
               (local.get $0)
               (local.get $7)
              )
              (i32.store offset=24
               (local.get $7)
               (local.get $0)
              )
              (i32.store offset=12
               (local.get $7)
               (local.get $7)
              )
              (i32.store offset=8
               (local.get $7)
               (local.get $7)
              )
              (br $label$break$L317)
             )
            )
            (if
             (i32.eq
              (local.get $2)
              (i32.and
               (i32.load offset=4
                (local.tee $0
                 (i32.load
                  (local.get $0)
                 )
                )
               )
               (i32.const -8)
              )
             )
             (local.set $11
              (local.get $0)
             )
             (block $label$break$L410
              (local.set $1
               (i32.shl
                (local.get $2)
                (select
                 (i32.const 0)
                 (i32.sub
                  (i32.const 25)
                  (i32.shr_u
                   (local.get $3)
                   (i32.const 1)
                  )
                 )
                 (i32.eq
                  (local.get $3)
                  (i32.const 31)
                 )
                )
               )
              )
              (loop $while-in64
               (if
                (local.tee $3
                 (i32.load
                  (local.tee $4
                   (i32.add
                    (i32.add
                     (local.get $0)
                     (i32.const 16)
                    )
                    (i32.shl
                     (i32.shr_u
                      (local.get $1)
                      (i32.const 31)
                     )
                     (i32.const 2)
                    )
                   )
                  )
                 )
                )
                (block
                 (local.set $1
                  (i32.shl
                   (local.get $1)
                   (i32.const 1)
                  )
                 )
                 (if
                  (i32.eq
                   (local.get $2)
                   (i32.and
                    (i32.load offset=4
                     (local.get $3)
                    )
                    (i32.const -8)
                   )
                  )
                  (block
                   (local.set $11
                    (local.get $3)
                   )
                   (br $label$break$L410)
                  )
                  (block
                   (local.set $0
                    (local.get $3)
                   )
                   (br $while-in64)
                  )
                 )
                )
               )
              )
              (if
               (i32.gt_u
                (i32.load
                 (i32.const 1040)
                )
                (local.get $4)
               )
               (call $_abort)
               (block
                (i32.store
                 (local.get $4)
                 (local.get $7)
                )
                (i32.store offset=24
                 (local.get $7)
                 (local.get $0)
                )
                (i32.store offset=12
                 (local.get $7)
                 (local.get $7)
                )
                (i32.store offset=8
                 (local.get $7)
                 (local.get $7)
                )
                (br $label$break$L317)
               )
              )
             )
            )
            (if
             (i32.and
              (i32.le_u
               (local.tee $0
                (i32.load
                 (i32.const 1040)
                )
               )
               (local.get $11)
              )
              (i32.le_u
               (local.get $0)
               (local.tee $0
                (i32.load
                 (local.tee $3
                  (i32.add
                   (local.get $11)
                   (i32.const 8)
                  )
                 )
                )
               )
              )
             )
             (block
              (i32.store offset=12
               (local.get $0)
               (local.get $7)
              )
              (i32.store
               (local.get $3)
               (local.get $7)
              )
              (i32.store offset=8
               (local.get $7)
               (local.get $0)
              )
              (i32.store offset=12
               (local.get $7)
               (local.get $11)
              )
              (i32.store offset=24
               (local.get $7)
               (i32.const 0)
              )
             )
             (call $_abort)
            )
           )
          )
          (global.set $STACKTOP
           (local.get $14)
          )
          (return
           (i32.add
            (local.get $8)
            (i32.const 8)
           )
          )
         )
        )
       )
       (local.set $2
        (i32.const 1472)
       )
       (loop $while-in66
        (block $while-out65
         (if
          (i32.le_u
           (local.tee $4
            (i32.load
             (local.get $2)
            )
           )
           (local.get $6)
          )
          (br_if $while-out65
           (i32.gt_u
            (local.tee $5
             (i32.add
              (local.get $4)
              (i32.load offset=4
               (local.get $2)
              )
             )
            )
            (local.get $6)
           )
          )
         )
         (local.set $2
          (i32.load offset=8
           (local.get $2)
          )
         )
         (br $while-in66)
        )
       )
       (local.set $2
        (i32.add
         (local.tee $4
          (i32.add
           (local.get $5)
           (i32.const -47)
          )
         )
         (i32.const 8)
        )
       )
       (local.set $4
        (i32.add
         (local.tee $2
          (select
           (local.get $6)
           (local.tee $2
            (i32.add
             (local.get $4)
             (select
              (i32.and
               (i32.sub
                (i32.const 0)
                (local.get $2)
               )
               (i32.const 7)
              )
              (i32.const 0)
              (i32.and
               (local.get $2)
               (i32.const 7)
              )
             )
            )
           )
           (i32.lt_u
            (local.get $2)
            (local.tee $9
             (i32.add
              (local.get $6)
              (i32.const 16)
             )
            )
           )
          )
         )
         (i32.const 8)
        )
       )
       (i32.store
        (i32.const 1048)
        (local.tee $10
         (i32.add
          (local.get $0)
          (local.tee $7
           (select
            (i32.and
             (i32.sub
              (i32.const 0)
              (local.tee $7
               (i32.add
                (local.get $0)
                (i32.const 8)
               )
              )
             )
             (i32.const 7)
            )
            (i32.const 0)
            (i32.and
             (local.get $7)
             (i32.const 7)
            )
           )
          )
         )
        )
       )
       (i32.store
        (i32.const 1036)
        (local.tee $7
         (i32.sub
          (local.tee $11
           (i32.add
            (local.get $1)
            (i32.const -40)
           )
          )
          (local.get $7)
         )
        )
       )
       (i32.store offset=4
        (local.get $10)
        (i32.or
         (local.get $7)
         (i32.const 1)
        )
       )
       (i32.store offset=4
        (i32.add
         (local.get $0)
         (local.get $11)
        )
        (i32.const 40)
       )
       (i32.store
        (i32.const 1052)
        (i32.load
         (i32.const 1512)
        )
       )
       (i32.store
        (local.tee $7
         (i32.add
          (local.get $2)
          (i32.const 4)
         )
        )
        (i32.const 27)
       )
       (i64.store align=4
        (local.get $4)
        (i64.load align=4
         (i32.const 1472)
        )
       )
       (i64.store offset=8 align=4
        (local.get $4)
        (i64.load align=4
         (i32.const 1480)
        )
       )
       (i32.store
        (i32.const 1472)
        (local.get $0)
       )
       (i32.store
        (i32.const 1476)
        (local.get $1)
       )
       (i32.store
        (i32.const 1484)
        (i32.const 0)
       )
       (i32.store
        (i32.const 1480)
        (local.get $4)
       )
       (local.set $0
        (i32.add
         (local.get $2)
         (i32.const 24)
        )
       )
       (loop $while-in68
        (i32.store
         (local.tee $1
          (i32.add
           (local.get $0)
           (i32.const 4)
          )
         )
         (i32.const 7)
        )
        (if
         (i32.lt_u
          (i32.add
           (local.get $0)
           (i32.const 8)
          )
          (local.get $5)
         )
         (block
          (local.set $0
           (local.get $1)
          )
          (br $while-in68)
         )
        )
       )
       (if
        (i32.ne
         (local.get $2)
         (local.get $6)
        )
        (block
         (i32.store
          (local.get $7)
          (i32.and
           (i32.load
            (local.get $7)
           )
           (i32.const -2)
          )
         )
         (i32.store offset=4
          (local.get $6)
          (i32.or
           (local.tee $4
            (i32.sub
             (local.get $2)
             (local.get $6)
            )
           )
           (i32.const 1)
          )
         )
         (i32.store
          (local.get $2)
          (local.get $4)
         )
         (local.set $1
          (i32.shr_u
           (local.get $4)
           (i32.const 3)
          )
         )
         (if
          (i32.lt_u
           (local.get $4)
           (i32.const 256)
          )
          (block
           (local.set $0
            (i32.add
             (i32.shl
              (local.get $1)
              (i32.const 3)
             )
             (i32.const 1064)
            )
           )
           (if
            (i32.and
             (local.tee $2
              (i32.load
               (i32.const 1024)
              )
             )
             (local.tee $1
              (i32.shl
               (i32.const 1)
               (local.get $1)
              )
             )
            )
            (if
             (i32.gt_u
              (i32.load
               (i32.const 1040)
              )
              (local.tee $2
               (i32.load
                (local.tee $1
                 (i32.add
                  (local.get $0)
                  (i32.const 8)
                 )
                )
               )
              )
             )
             (call $_abort)
             (block
              (local.set $17
               (local.get $2)
              )
              (local.set $22
               (local.get $1)
              )
             )
            )
            (block
             (i32.store
              (i32.const 1024)
              (i32.or
               (local.get $1)
               (local.get $2)
              )
             )
             (local.set $17
              (local.get $0)
             )
             (local.set $22
              (i32.add
               (local.get $0)
               (i32.const 8)
              )
             )
            )
           )
           (i32.store
            (local.get $22)
            (local.get $6)
           )
           (i32.store offset=12
            (local.get $17)
            (local.get $6)
           )
           (i32.store offset=8
            (local.get $6)
            (local.get $17)
           )
           (i32.store offset=12
            (local.get $6)
            (local.get $0)
           )
           (br $label$break$L294)
          )
         )
         (local.set $0
          (i32.add
           (i32.shl
            (local.tee $1
             (if (result i32)
              (local.tee $0
               (i32.shr_u
                (local.get $4)
                (i32.const 8)
               )
              )
              (if (result i32)
               (i32.gt_u
                (local.get $4)
                (i32.const 16777215)
               )
               (i32.const 31)
               (block (result i32)
                (local.set $0
                 (i32.and
                  (i32.shr_u
                   (i32.add
                    (local.tee $2
                     (i32.shl
                      (local.get $0)
                      (local.tee $1
                       (i32.and
                        (i32.shr_u
                         (i32.add
                          (local.get $0)
                          (i32.const 1048320)
                         )
                         (i32.const 16)
                        )
                        (i32.const 8)
                       )
                      )
                     )
                    )
                    (i32.const 520192)
                   )
                   (i32.const 16)
                  )
                  (i32.const 4)
                 )
                )
                (i32.or
                 (i32.shl
                  (local.tee $0
                   (i32.add
                    (i32.sub
                     (i32.const 14)
                     (i32.or
                      (local.tee $5
                       (i32.and
                        (i32.shr_u
                         (i32.add
                          (local.tee $2
                           (i32.shl
                            (local.get $2)
                            (local.get $0)
                           )
                          )
                          (i32.const 245760)
                         )
                         (i32.const 16)
                        )
                        (i32.const 2)
                       )
                      )
                      (i32.or
                       (local.get $0)
                       (local.get $1)
                      )
                     )
                    )
                    (i32.shr_u
                     (i32.shl
                      (local.get $2)
                      (local.get $5)
                     )
                     (i32.const 15)
                    )
                   )
                  )
                  (i32.const 1)
                 )
                 (i32.and
                  (i32.shr_u
                   (local.get $4)
                   (i32.add
                    (local.get $0)
                    (i32.const 7)
                   )
                  )
                  (i32.const 1)
                 )
                )
               )
              )
              (i32.const 0)
             )
            )
            (i32.const 2)
           )
           (i32.const 1328)
          )
         )
         (i32.store offset=28
          (local.get $6)
          (local.get $1)
         )
         (i32.store offset=20
          (local.get $6)
          (i32.const 0)
         )
         (i32.store
          (local.get $9)
          (i32.const 0)
         )
         (if
          (i32.eqz
           (i32.and
            (local.tee $2
             (i32.load
              (i32.const 1028)
             )
            )
            (local.tee $5
             (i32.shl
              (i32.const 1)
              (local.get $1)
             )
            )
           )
          )
          (block
           (i32.store
            (i32.const 1028)
            (i32.or
             (local.get $2)
             (local.get $5)
            )
           )
           (i32.store
            (local.get $0)
            (local.get $6)
           )
           (i32.store offset=24
            (local.get $6)
            (local.get $0)
           )
           (i32.store offset=12
            (local.get $6)
            (local.get $6)
           )
           (i32.store offset=8
            (local.get $6)
            (local.get $6)
           )
           (br $label$break$L294)
          )
         )
         (if
          (i32.eq
           (i32.and
            (i32.load offset=4
             (local.tee $0
              (i32.load
               (local.get $0)
              )
             )
            )
            (i32.const -8)
           )
           (local.get $4)
          )
          (local.set $8
           (local.get $0)
          )
          (block $label$break$L451
           (local.set $2
            (i32.shl
             (local.get $4)
             (select
              (i32.const 0)
              (i32.sub
               (i32.const 25)
               (i32.shr_u
                (local.get $1)
                (i32.const 1)
               )
              )
              (i32.eq
               (local.get $1)
               (i32.const 31)
              )
             )
            )
           )
           (loop $while-in71
            (if
             (local.tee $1
              (i32.load
               (local.tee $5
                (i32.add
                 (i32.add
                  (local.get $0)
                  (i32.const 16)
                 )
                 (i32.shl
                  (i32.shr_u
                   (local.get $2)
                   (i32.const 31)
                  )
                  (i32.const 2)
                 )
                )
               )
              )
             )
             (block
              (local.set $2
               (i32.shl
                (local.get $2)
                (i32.const 1)
               )
              )
              (if
               (i32.eq
                (i32.and
                 (i32.load offset=4
                  (local.get $1)
                 )
                 (i32.const -8)
                )
                (local.get $4)
               )
               (block
                (local.set $8
                 (local.get $1)
                )
                (br $label$break$L451)
               )
               (block
                (local.set $0
                 (local.get $1)
                )
                (br $while-in71)
               )
              )
             )
            )
           )
           (if
            (i32.gt_u
             (i32.load
              (i32.const 1040)
             )
             (local.get $5)
            )
            (call $_abort)
            (block
             (i32.store
              (local.get $5)
              (local.get $6)
             )
             (i32.store offset=24
              (local.get $6)
              (local.get $0)
             )
             (i32.store offset=12
              (local.get $6)
              (local.get $6)
             )
             (i32.store offset=8
              (local.get $6)
              (local.get $6)
             )
             (br $label$break$L294)
            )
           )
          )
         )
         (if
          (i32.and
           (i32.le_u
            (local.tee $0
             (i32.load
              (i32.const 1040)
             )
            )
            (local.get $8)
           )
           (i32.le_u
            (local.get $0)
            (local.tee $0
             (i32.load
              (local.tee $1
               (i32.add
                (local.get $8)
                (i32.const 8)
               )
              )
             )
            )
           )
          )
          (block
           (i32.store offset=12
            (local.get $0)
            (local.get $6)
           )
           (i32.store
            (local.get $1)
            (local.get $6)
           )
           (i32.store offset=8
            (local.get $6)
            (local.get $0)
           )
           (i32.store offset=12
            (local.get $6)
            (local.get $8)
           )
           (i32.store offset=24
            (local.get $6)
            (i32.const 0)
           )
          )
          (call $_abort)
         )
        )
       )
      )
      (block
       (if
        (i32.or
         (i32.eqz
          (local.tee $2
           (i32.load
            (i32.const 1040)
           )
          )
         )
         (i32.lt_u
          (local.get $0)
          (local.get $2)
         )
        )
        (i32.store
         (i32.const 1040)
         (local.get $0)
        )
       )
       (i32.store
        (i32.const 1472)
        (local.get $0)
       )
       (i32.store
        (i32.const 1476)
        (local.get $1)
       )
       (i32.store
        (i32.const 1484)
        (i32.const 0)
       )
       (i32.store
        (i32.const 1060)
        (i32.load
         (i32.const 1496)
        )
       )
       (i32.store
        (i32.const 1056)
        (i32.const -1)
       )
       (i32.store
        (i32.const 1076)
        (i32.const 1064)
       )
       (i32.store
        (i32.const 1072)
        (i32.const 1064)
       )
       (i32.store
        (i32.const 1084)
        (i32.const 1072)
       )
       (i32.store
        (i32.const 1080)
        (i32.const 1072)
       )
       (i32.store
        (i32.const 1092)
        (i32.const 1080)
       )
       (i32.store
        (i32.const 1088)
        (i32.const 1080)
       )
       (i32.store
        (i32.const 1100)
        (i32.const 1088)
       )
       (i32.store
        (i32.const 1096)
        (i32.const 1088)
       )
       (i32.store
        (i32.const 1108)
        (i32.const 1096)
       )
       (i32.store
        (i32.const 1104)
        (i32.const 1096)
       )
       (i32.store
        (i32.const 1116)
        (i32.const 1104)
       )
       (i32.store
        (i32.const 1112)
        (i32.const 1104)
       )
       (i32.store
        (i32.const 1124)
        (i32.const 1112)
       )
       (i32.store
        (i32.const 1120)
        (i32.const 1112)
       )
       (i32.store
        (i32.const 1132)
        (i32.const 1120)
       )
       (i32.store
        (i32.const 1128)
        (i32.const 1120)
       )
       (i32.store
        (i32.const 1140)
        (i32.const 1128)
       )
       (i32.store
        (i32.const 1136)
        (i32.const 1128)
       )
       (i32.store
        (i32.const 1148)
        (i32.const 1136)
       )
       (i32.store
        (i32.const 1144)
        (i32.const 1136)
       )
       (i32.store
        (i32.const 1156)
        (i32.const 1144)
       )
       (i32.store
        (i32.const 1152)
        (i32.const 1144)
       )
       (i32.store
        (i32.const 1164)
        (i32.const 1152)
       )
       (i32.store
        (i32.const 1160)
        (i32.const 1152)
       )
       (i32.store
        (i32.const 1172)
        (i32.const 1160)
       )
       (i32.store
        (i32.const 1168)
        (i32.const 1160)
       )
       (i32.store
        (i32.const 1180)
        (i32.const 1168)
       )
       (i32.store
        (i32.const 1176)
        (i32.const 1168)
       )
       (i32.store
        (i32.const 1188)
        (i32.const 1176)
       )
       (i32.store
        (i32.const 1184)
        (i32.const 1176)
       )
       (i32.store
        (i32.const 1196)
        (i32.const 1184)
       )
       (i32.store
        (i32.const 1192)
        (i32.const 1184)
       )
       (i32.store
        (i32.const 1204)
        (i32.const 1192)
       )
       (i32.store
        (i32.const 1200)
        (i32.const 1192)
       )
       (i32.store
        (i32.const 1212)
        (i32.const 1200)
       )
       (i32.store
        (i32.const 1208)
        (i32.const 1200)
       )
       (i32.store
        (i32.const 1220)
        (i32.const 1208)
       )
       (i32.store
        (i32.const 1216)
        (i32.const 1208)
       )
       (i32.store
        (i32.const 1228)
        (i32.const 1216)
       )
       (i32.store
        (i32.const 1224)
        (i32.const 1216)
       )
       (i32.store
        (i32.const 1236)
        (i32.const 1224)
       )
       (i32.store
        (i32.const 1232)
        (i32.const 1224)
       )
       (i32.store
        (i32.const 1244)
        (i32.const 1232)
       )
       (i32.store
        (i32.const 1240)
        (i32.const 1232)
       )
       (i32.store
        (i32.const 1252)
        (i32.const 1240)
       )
       (i32.store
        (i32.const 1248)
        (i32.const 1240)
       )
       (i32.store
        (i32.const 1260)
        (i32.const 1248)
       )
       (i32.store
        (i32.const 1256)
        (i32.const 1248)
       )
       (i32.store
        (i32.const 1268)
        (i32.const 1256)
       )
       (i32.store
        (i32.const 1264)
        (i32.const 1256)
       )
       (i32.store
        (i32.const 1276)
        (i32.const 1264)
       )
       (i32.store
        (i32.const 1272)
        (i32.const 1264)
       )
       (i32.store
        (i32.const 1284)
        (i32.const 1272)
       )
       (i32.store
        (i32.const 1280)
        (i32.const 1272)
       )
       (i32.store
        (i32.const 1292)
        (i32.const 1280)
       )
       (i32.store
        (i32.const 1288)
        (i32.const 1280)
       )
       (i32.store
        (i32.const 1300)
        (i32.const 1288)
       )
       (i32.store
        (i32.const 1296)
        (i32.const 1288)
       )
       (i32.store
        (i32.const 1308)
        (i32.const 1296)
       )
       (i32.store
        (i32.const 1304)
        (i32.const 1296)
       )
       (i32.store
        (i32.const 1316)
        (i32.const 1304)
       )
       (i32.store
        (i32.const 1312)
        (i32.const 1304)
       )
       (i32.store
        (i32.const 1324)
        (i32.const 1312)
       )
       (i32.store
        (i32.const 1320)
        (i32.const 1312)
       )
       (i32.store
        (i32.const 1048)
        (local.tee $4
         (i32.add
          (local.get $0)
          (local.tee $2
           (select
            (i32.and
             (i32.sub
              (i32.const 0)
              (local.tee $2
               (i32.add
                (local.get $0)
                (i32.const 8)
               )
              )
             )
             (i32.const 7)
            )
            (i32.const 0)
            (i32.and
             (local.get $2)
             (i32.const 7)
            )
           )
          )
         )
        )
       )
       (i32.store
        (i32.const 1036)
        (local.tee $2
         (i32.sub
          (local.tee $1
           (i32.add
            (local.get $1)
            (i32.const -40)
           )
          )
          (local.get $2)
         )
        )
       )
       (i32.store offset=4
        (local.get $4)
        (i32.or
         (local.get $2)
         (i32.const 1)
        )
       )
       (i32.store offset=4
        (i32.add
         (local.get $0)
         (local.get $1)
        )
        (i32.const 40)
       )
       (i32.store
        (i32.const 1052)
        (i32.load
         (i32.const 1512)
        )
       )
      )
     )
     (if
      (i32.gt_u
       (local.tee $0
        (i32.load
         (i32.const 1036)
        )
       )
       (local.get $3)
      )
      (block
       (i32.store
        (i32.const 1036)
        (local.tee $1
         (i32.sub
          (local.get $0)
          (local.get $3)
         )
        )
       )
       (br $folding-inner0)
      )
     )
    )
    (i32.store
     (call $___errno_location)
     (i32.const 12)
    )
    (global.set $STACKTOP
     (local.get $14)
    )
    (return
     (i32.const 0)
    )
   )
   (i32.store
    (i32.const 1048)
    (local.tee $2
     (i32.add
      (local.get $3)
      (local.tee $0
       (i32.load
        (i32.const 1048)
       )
      )
     )
    )
   )
   (i32.store offset=4
    (local.get $2)
    (i32.or
     (local.get $1)
     (i32.const 1)
    )
   )
   (i32.store offset=4
    (local.get $0)
    (i32.or
     (local.get $3)
     (i32.const 3)
    )
   )
  )
  (global.set $STACKTOP
   (local.get $14)
  )
  (i32.add
   (local.get $0)
   (i32.const 8)
  )
 )
 (func $_free (; 13 ;) (; has Stack IR ;) (param $0 i32)
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
  (if
   (i32.eqz
    (local.get $0)
   )
   (return)
  )
  (if
   (i32.lt_u
    (local.tee $4
     (i32.add
      (local.get $0)
      (i32.const -8)
     )
    )
    (local.tee $12
     (i32.load
      (i32.const 1040)
     )
    )
   )
   (call $_abort)
  )
  (if
   (i32.eq
    (local.tee $11
     (i32.and
      (local.tee $0
       (i32.load
        (i32.add
         (local.get $0)
         (i32.const -4)
        )
       )
      )
      (i32.const 3)
     )
    )
    (i32.const 1)
   )
   (call $_abort)
  )
  (local.set $7
   (i32.add
    (local.get $4)
    (local.tee $2
     (i32.and
      (local.get $0)
      (i32.const -8)
     )
    )
   )
  )
  (if
   (i32.and
    (local.get $0)
    (i32.const 1)
   )
   (block
    (local.set $1
     (local.get $2)
    )
    (local.set $5
     (local.tee $3
      (local.get $4)
     )
    )
   )
   (block $label$break$L10
    (local.set $9
     (i32.load
      (local.get $4)
     )
    )
    (if
     (i32.eqz
      (local.get $11)
     )
     (return)
    )
    (if
     (i32.lt_u
      (local.tee $0
       (i32.sub
        (local.get $4)
        (local.get $9)
       )
      )
      (local.get $12)
     )
     (call $_abort)
    )
    (local.set $4
     (i32.add
      (local.get $2)
      (local.get $9)
     )
    )
    (if
     (i32.eq
      (local.get $0)
      (i32.load
       (i32.const 1044)
      )
     )
     (block
      (if
       (i32.ne
        (i32.and
         (local.tee $1
          (i32.load
           (local.tee $3
            (i32.add
             (local.get $7)
             (i32.const 4)
            )
           )
          )
         )
         (i32.const 3)
        )
        (i32.const 3)
       )
       (block
        (local.set $1
         (local.get $4)
        )
        (local.set $5
         (local.tee $3
          (local.get $0)
         )
        )
        (br $label$break$L10)
       )
      )
      (i32.store
       (i32.const 1032)
       (local.get $4)
      )
      (i32.store
       (local.get $3)
       (i32.and
        (local.get $1)
        (i32.const -2)
       )
      )
      (i32.store offset=4
       (local.get $0)
       (i32.or
        (local.get $4)
        (i32.const 1)
       )
      )
      (i32.store
       (i32.add
        (local.get $0)
        (local.get $4)
       )
       (local.get $4)
      )
      (return)
     )
    )
    (local.set $2
     (i32.shr_u
      (local.get $9)
      (i32.const 3)
     )
    )
    (if
     (i32.lt_u
      (local.get $9)
      (i32.const 256)
     )
     (block
      (local.set $1
       (i32.load offset=12
        (local.get $0)
       )
      )
      (if
       (i32.ne
        (local.tee $5
         (i32.load offset=8
          (local.get $0)
         )
        )
        (local.tee $3
         (i32.add
          (i32.shl
           (local.get $2)
           (i32.const 3)
          )
          (i32.const 1064)
         )
        )
       )
       (block
        (if
         (i32.gt_u
          (local.get $12)
          (local.get $5)
         )
         (call $_abort)
        )
        (if
         (i32.ne
          (i32.load offset=12
           (local.get $5)
          )
          (local.get $0)
         )
         (call $_abort)
        )
       )
      )
      (if
       (i32.eq
        (local.get $1)
        (local.get $5)
       )
       (block
        (i32.store
         (i32.const 1024)
         (i32.and
          (i32.load
           (i32.const 1024)
          )
          (i32.xor
           (i32.shl
            (i32.const 1)
            (local.get $2)
           )
           (i32.const -1)
          )
         )
        )
        (local.set $1
         (local.get $4)
        )
        (local.set $5
         (local.tee $3
          (local.get $0)
         )
        )
        (br $label$break$L10)
       )
      )
      (if
       (i32.eq
        (local.get $1)
        (local.get $3)
       )
       (local.set $6
        (i32.add
         (local.get $1)
         (i32.const 8)
        )
       )
       (block
        (if
         (i32.gt_u
          (local.get $12)
          (local.get $1)
         )
         (call $_abort)
        )
        (if
         (i32.eq
          (local.get $0)
          (i32.load
           (local.tee $3
            (i32.add
             (local.get $1)
             (i32.const 8)
            )
           )
          )
         )
         (local.set $6
          (local.get $3)
         )
         (call $_abort)
        )
       )
      )
      (i32.store offset=12
       (local.get $5)
       (local.get $1)
      )
      (i32.store
       (local.get $6)
       (local.get $5)
      )
      (local.set $1
       (local.get $4)
      )
      (local.set $5
       (local.tee $3
        (local.get $0)
       )
      )
      (br $label$break$L10)
     )
    )
    (local.set $13
     (i32.load offset=24
      (local.get $0)
     )
    )
    (if
     (i32.eq
      (local.get $0)
      (local.tee $2
       (i32.load offset=12
        (local.get $0)
       )
      )
     )
     (block $do-once
      (if
       (local.tee $2
        (i32.load
         (local.tee $9
          (i32.add
           (local.tee $6
            (i32.add
             (local.get $0)
             (i32.const 16)
            )
           )
           (i32.const 4)
          )
         )
        )
       )
       (local.set $6
        (local.get $9)
       )
       (br_if $do-once
        (i32.eqz
         (local.tee $2
          (i32.load
           (local.get $6)
          )
         )
        )
       )
      )
      (loop $while-in
       (block $while-out
        (if
         (i32.eqz
          (local.tee $11
           (i32.load
            (local.tee $9
             (i32.add
              (local.get $2)
              (i32.const 20)
             )
            )
           )
          )
         )
         (br_if $while-out
          (i32.eqz
           (local.tee $11
            (i32.load
             (local.tee $9
              (i32.add
               (local.get $2)
               (i32.const 16)
              )
             )
            )
           )
          )
         )
        )
        (local.set $6
         (local.get $9)
        )
        (local.set $2
         (local.get $11)
        )
        (br $while-in)
       )
      )
      (if
       (i32.gt_u
        (local.get $12)
        (local.get $6)
       )
       (call $_abort)
       (block
        (i32.store
         (local.get $6)
         (i32.const 0)
        )
        (local.set $8
         (local.get $2)
        )
       )
      )
     )
     (block
      (if
       (i32.gt_u
        (local.get $12)
        (local.tee $6
         (i32.load offset=8
          (local.get $0)
         )
        )
       )
       (call $_abort)
      )
      (if
       (i32.ne
        (i32.load
         (local.tee $9
          (i32.add
           (local.get $6)
           (i32.const 12)
          )
         )
        )
        (local.get $0)
       )
       (call $_abort)
      )
      (if
       (i32.eq
        (local.get $0)
        (i32.load
         (local.tee $11
          (i32.add
           (local.get $2)
           (i32.const 8)
          )
         )
        )
       )
       (block
        (i32.store
         (local.get $9)
         (local.get $2)
        )
        (i32.store
         (local.get $11)
         (local.get $6)
        )
        (local.set $8
         (local.get $2)
        )
       )
       (call $_abort)
      )
     )
    )
    (if
     (local.get $13)
     (block
      (if
       (i32.eq
        (local.get $0)
        (i32.load
         (local.tee $6
          (i32.add
           (i32.shl
            (local.tee $2
             (i32.load offset=28
              (local.get $0)
             )
            )
            (i32.const 2)
           )
           (i32.const 1328)
          )
         )
        )
       )
       (block
        (i32.store
         (local.get $6)
         (local.get $8)
        )
        (if
         (i32.eqz
          (local.get $8)
         )
         (block
          (i32.store
           (i32.const 1028)
           (i32.and
            (i32.load
             (i32.const 1028)
            )
            (i32.xor
             (i32.shl
              (i32.const 1)
              (local.get $2)
             )
             (i32.const -1)
            )
           )
          )
          (local.set $1
           (local.get $4)
          )
          (local.set $5
           (local.tee $3
            (local.get $0)
           )
          )
          (br $label$break$L10)
         )
        )
       )
       (if
        (i32.gt_u
         (i32.load
          (i32.const 1040)
         )
         (local.get $13)
        )
        (call $_abort)
        (block
         (i32.store
          (select
           (local.tee $2
            (i32.add
             (local.get $13)
             (i32.const 16)
            )
           )
           (i32.add
            (local.get $13)
            (i32.const 20)
           )
           (i32.eq
            (local.get $0)
            (i32.load
             (local.get $2)
            )
           )
          )
          (local.get $8)
         )
         (if
          (i32.eqz
           (local.get $8)
          )
          (block
           (local.set $1
            (local.get $4)
           )
           (local.set $5
            (local.tee $3
             (local.get $0)
            )
           )
           (br $label$break$L10)
          )
         )
        )
       )
      )
      (if
       (i32.gt_u
        (local.tee $6
         (i32.load
          (i32.const 1040)
         )
        )
        (local.get $8)
       )
       (call $_abort)
      )
      (i32.store offset=24
       (local.get $8)
       (local.get $13)
      )
      (if
       (local.tee $2
        (i32.load
         (local.tee $9
          (i32.add
           (local.get $0)
           (i32.const 16)
          )
         )
        )
       )
       (if
        (i32.gt_u
         (local.get $6)
         (local.get $2)
        )
        (call $_abort)
        (block
         (i32.store offset=16
          (local.get $8)
          (local.get $2)
         )
         (i32.store offset=24
          (local.get $2)
          (local.get $8)
         )
        )
       )
      )
      (if
       (local.tee $2
        (i32.load offset=4
         (local.get $9)
        )
       )
       (if
        (i32.gt_u
         (i32.load
          (i32.const 1040)
         )
         (local.get $2)
        )
        (call $_abort)
        (block
         (i32.store offset=20
          (local.get $8)
          (local.get $2)
         )
         (i32.store offset=24
          (local.get $2)
          (local.get $8)
         )
         (local.set $1
          (local.get $4)
         )
         (local.set $5
          (local.tee $3
           (local.get $0)
          )
         )
        )
       )
       (block
        (local.set $1
         (local.get $4)
        )
        (local.set $5
         (local.tee $3
          (local.get $0)
         )
        )
       )
      )
     )
     (block
      (local.set $1
       (local.get $4)
      )
      (local.set $5
       (local.tee $3
        (local.get $0)
       )
      )
     )
    )
   )
  )
  (if
   (i32.ge_u
    (local.get $5)
    (local.get $7)
   )
   (call $_abort)
  )
  (if
   (i32.eqz
    (i32.and
     (local.tee $0
      (i32.load
       (local.tee $4
        (i32.add
         (local.get $7)
         (i32.const 4)
        )
       )
      )
     )
     (i32.const 1)
    )
   )
   (call $_abort)
  )
  (local.set $1
   (i32.shr_u
    (local.tee $5
     (if (result i32)
      (i32.and
       (local.get $0)
       (i32.const 2)
      )
      (block (result i32)
       (i32.store
        (local.get $4)
        (i32.and
         (local.get $0)
         (i32.const -2)
        )
       )
       (i32.store offset=4
        (local.get $3)
        (i32.or
         (local.get $1)
         (i32.const 1)
        )
       )
       (i32.store
        (i32.add
         (local.get $1)
         (local.get $5)
        )
        (local.get $1)
       )
       (local.get $1)
      )
      (block (result i32)
       (if
        (i32.eq
         (local.get $7)
         (i32.load
          (i32.const 1048)
         )
        )
        (block
         (i32.store
          (i32.const 1036)
          (local.tee $0
           (i32.add
            (local.get $1)
            (i32.load
             (i32.const 1036)
            )
           )
          )
         )
         (i32.store
          (i32.const 1048)
          (local.get $3)
         )
         (i32.store offset=4
          (local.get $3)
          (i32.or
           (local.get $0)
           (i32.const 1)
          )
         )
         (if
          (i32.ne
           (i32.load
            (i32.const 1044)
           )
           (local.get $3)
          )
          (return)
         )
         (i32.store
          (i32.const 1044)
          (i32.const 0)
         )
         (i32.store
          (i32.const 1032)
          (i32.const 0)
         )
         (return)
        )
       )
       (if
        (i32.eq
         (local.get $7)
         (i32.load
          (i32.const 1044)
         )
        )
        (block
         (i32.store
          (i32.const 1032)
          (local.tee $0
           (i32.add
            (local.get $1)
            (i32.load
             (i32.const 1032)
            )
           )
          )
         )
         (i32.store
          (i32.const 1044)
          (local.get $5)
         )
         (i32.store offset=4
          (local.get $3)
          (i32.or
           (local.get $0)
           (i32.const 1)
          )
         )
         (i32.store
          (i32.add
           (local.get $0)
           (local.get $5)
          )
          (local.get $0)
         )
         (return)
        )
       )
       (local.set $4
        (i32.add
         (local.get $1)
         (i32.and
          (local.get $0)
          (i32.const -8)
         )
        )
       )
       (local.set $6
        (i32.shr_u
         (local.get $0)
         (i32.const 3)
        )
       )
       (block $label$break$L111
        (if
         (i32.lt_u
          (local.get $0)
          (i32.const 256)
         )
         (block
          (local.set $1
           (i32.load offset=12
            (local.get $7)
           )
          )
          (if
           (i32.ne
            (local.tee $2
             (i32.load offset=8
              (local.get $7)
             )
            )
            (local.tee $0
             (i32.add
              (i32.shl
               (local.get $6)
               (i32.const 3)
              )
              (i32.const 1064)
             )
            )
           )
           (block
            (if
             (i32.gt_u
              (i32.load
               (i32.const 1040)
              )
              (local.get $2)
             )
             (call $_abort)
            )
            (if
             (i32.ne
              (i32.load offset=12
               (local.get $2)
              )
              (local.get $7)
             )
             (call $_abort)
            )
           )
          )
          (if
           (i32.eq
            (local.get $1)
            (local.get $2)
           )
           (block
            (i32.store
             (i32.const 1024)
             (i32.and
              (i32.load
               (i32.const 1024)
              )
              (i32.xor
               (i32.shl
                (i32.const 1)
                (local.get $6)
               )
               (i32.const -1)
              )
             )
            )
            (br $label$break$L111)
           )
          )
          (if
           (i32.eq
            (local.get $0)
            (local.get $1)
           )
           (local.set $16
            (i32.add
             (local.get $1)
             (i32.const 8)
            )
           )
           (block
            (if
             (i32.gt_u
              (i32.load
               (i32.const 1040)
              )
              (local.get $1)
             )
             (call $_abort)
            )
            (if
             (i32.eq
              (local.get $7)
              (i32.load
               (local.tee $0
                (i32.add
                 (local.get $1)
                 (i32.const 8)
                )
               )
              )
             )
             (local.set $16
              (local.get $0)
             )
             (call $_abort)
            )
           )
          )
          (i32.store offset=12
           (local.get $2)
           (local.get $1)
          )
          (i32.store
           (local.get $16)
           (local.get $2)
          )
         )
         (block
          (local.set $8
           (i32.load offset=24
            (local.get $7)
           )
          )
          (if
           (i32.eq
            (local.get $7)
            (local.tee $0
             (i32.load offset=12
              (local.get $7)
             )
            )
           )
           (block $do-once6
            (if
             (local.tee $0
              (i32.load
               (local.tee $2
                (i32.add
                 (local.tee $1
                  (i32.add
                   (local.get $7)
                   (i32.const 16)
                  )
                 )
                 (i32.const 4)
                )
               )
              )
             )
             (local.set $1
              (local.get $2)
             )
             (br_if $do-once6
              (i32.eqz
               (local.tee $0
                (i32.load
                 (local.get $1)
                )
               )
              )
             )
            )
            (loop $while-in9
             (block $while-out8
              (if
               (i32.eqz
                (local.tee $6
                 (i32.load
                  (local.tee $2
                   (i32.add
                    (local.get $0)
                    (i32.const 20)
                   )
                  )
                 )
                )
               )
               (br_if $while-out8
                (i32.eqz
                 (local.tee $6
                  (i32.load
                   (local.tee $2
                    (i32.add
                     (local.get $0)
                     (i32.const 16)
                    )
                   )
                  )
                 )
                )
               )
              )
              (local.set $1
               (local.get $2)
              )
              (local.set $0
               (local.get $6)
              )
              (br $while-in9)
             )
            )
            (if
             (i32.gt_u
              (i32.load
               (i32.const 1040)
              )
              (local.get $1)
             )
             (call $_abort)
             (block
              (i32.store
               (local.get $1)
               (i32.const 0)
              )
              (local.set $10
               (local.get $0)
              )
             )
            )
           )
           (block
            (if
             (i32.gt_u
              (i32.load
               (i32.const 1040)
              )
              (local.tee $1
               (i32.load offset=8
                (local.get $7)
               )
              )
             )
             (call $_abort)
            )
            (if
             (i32.ne
              (i32.load
               (local.tee $2
                (i32.add
                 (local.get $1)
                 (i32.const 12)
                )
               )
              )
              (local.get $7)
             )
             (call $_abort)
            )
            (if
             (i32.eq
              (local.get $7)
              (i32.load
               (local.tee $6
                (i32.add
                 (local.get $0)
                 (i32.const 8)
                )
               )
              )
             )
             (block
              (i32.store
               (local.get $2)
               (local.get $0)
              )
              (i32.store
               (local.get $6)
               (local.get $1)
              )
              (local.set $10
               (local.get $0)
              )
             )
             (call $_abort)
            )
           )
          )
          (if
           (local.get $8)
           (block
            (if
             (i32.eq
              (local.get $7)
              (i32.load
               (local.tee $1
                (i32.add
                 (i32.shl
                  (local.tee $0
                   (i32.load offset=28
                    (local.get $7)
                   )
                  )
                  (i32.const 2)
                 )
                 (i32.const 1328)
                )
               )
              )
             )
             (block
              (i32.store
               (local.get $1)
               (local.get $10)
              )
              (if
               (i32.eqz
                (local.get $10)
               )
               (block
                (i32.store
                 (i32.const 1028)
                 (i32.and
                  (i32.load
                   (i32.const 1028)
                  )
                  (i32.xor
                   (i32.shl
                    (i32.const 1)
                    (local.get $0)
                   )
                   (i32.const -1)
                  )
                 )
                )
                (br $label$break$L111)
               )
              )
             )
             (if
              (i32.gt_u
               (i32.load
                (i32.const 1040)
               )
               (local.get $8)
              )
              (call $_abort)
              (block
               (i32.store
                (select
                 (local.tee $0
                  (i32.add
                   (local.get $8)
                   (i32.const 16)
                  )
                 )
                 (i32.add
                  (local.get $8)
                  (i32.const 20)
                 )
                 (i32.eq
                  (local.get $7)
                  (i32.load
                   (local.get $0)
                  )
                 )
                )
                (local.get $10)
               )
               (br_if $label$break$L111
                (i32.eqz
                 (local.get $10)
                )
               )
              )
             )
            )
            (if
             (i32.gt_u
              (local.tee $1
               (i32.load
                (i32.const 1040)
               )
              )
              (local.get $10)
             )
             (call $_abort)
            )
            (i32.store offset=24
             (local.get $10)
             (local.get $8)
            )
            (if
             (local.tee $0
              (i32.load
               (local.tee $2
                (i32.add
                 (local.get $7)
                 (i32.const 16)
                )
               )
              )
             )
             (if
              (i32.gt_u
               (local.get $1)
               (local.get $0)
              )
              (call $_abort)
              (block
               (i32.store offset=16
                (local.get $10)
                (local.get $0)
               )
               (i32.store offset=24
                (local.get $0)
                (local.get $10)
               )
              )
             )
            )
            (if
             (local.tee $0
              (i32.load offset=4
               (local.get $2)
              )
             )
             (if
              (i32.gt_u
               (i32.load
                (i32.const 1040)
               )
               (local.get $0)
              )
              (call $_abort)
              (block
               (i32.store offset=20
                (local.get $10)
                (local.get $0)
               )
               (i32.store offset=24
                (local.get $0)
                (local.get $10)
               )
              )
             )
            )
           )
          )
         )
        )
       )
       (i32.store offset=4
        (local.get $3)
        (i32.or
         (local.get $4)
         (i32.const 1)
        )
       )
       (i32.store
        (i32.add
         (local.get $4)
         (local.get $5)
        )
        (local.get $4)
       )
       (if (result i32)
        (i32.eq
         (local.get $3)
         (i32.load
          (i32.const 1044)
         )
        )
        (block
         (i32.store
          (i32.const 1032)
          (local.get $4)
         )
         (return)
        )
        (local.get $4)
       )
      )
     )
    )
    (i32.const 3)
   )
  )
  (if
   (i32.lt_u
    (local.get $5)
    (i32.const 256)
   )
   (block
    (local.set $0
     (i32.add
      (i32.shl
       (local.get $1)
       (i32.const 3)
      )
      (i32.const 1064)
     )
    )
    (if
     (i32.and
      (local.tee $5
       (i32.load
        (i32.const 1024)
       )
      )
      (local.tee $1
       (i32.shl
        (i32.const 1)
        (local.get $1)
       )
      )
     )
     (if
      (i32.gt_u
       (i32.load
        (i32.const 1040)
       )
       (local.tee $5
        (i32.load
         (local.tee $1
          (i32.add
           (local.get $0)
           (i32.const 8)
          )
         )
        )
       )
      )
      (call $_abort)
      (block
       (local.set $15
        (local.get $5)
       )
       (local.set $17
        (local.get $1)
       )
      )
     )
     (block
      (i32.store
       (i32.const 1024)
       (i32.or
        (local.get $1)
        (local.get $5)
       )
      )
      (local.set $15
       (local.get $0)
      )
      (local.set $17
       (i32.add
        (local.get $0)
        (i32.const 8)
       )
      )
     )
    )
    (i32.store
     (local.get $17)
     (local.get $3)
    )
    (i32.store offset=12
     (local.get $15)
     (local.get $3)
    )
    (i32.store offset=8
     (local.get $3)
     (local.get $15)
    )
    (i32.store offset=12
     (local.get $3)
     (local.get $0)
    )
    (return)
   )
  )
  (local.set $0
   (i32.add
    (i32.shl
     (local.tee $1
      (if (result i32)
       (local.tee $0
        (i32.shr_u
         (local.get $5)
         (i32.const 8)
        )
       )
       (if (result i32)
        (i32.gt_u
         (local.get $5)
         (i32.const 16777215)
        )
        (i32.const 31)
        (block (result i32)
         (local.set $0
          (i32.and
           (i32.shr_u
            (i32.add
             (local.tee $4
              (i32.shl
               (local.get $0)
               (local.tee $1
                (i32.and
                 (i32.shr_u
                  (i32.add
                   (local.get $0)
                   (i32.const 1048320)
                  )
                  (i32.const 16)
                 )
                 (i32.const 8)
                )
               )
              )
             )
             (i32.const 520192)
            )
            (i32.const 16)
           )
           (i32.const 4)
          )
         )
         (i32.or
          (i32.shl
           (local.tee $0
            (i32.add
             (i32.sub
              (i32.const 14)
              (i32.or
               (i32.or
                (local.get $0)
                (local.get $1)
               )
               (local.tee $1
                (i32.and
                 (i32.shr_u
                  (i32.add
                   (local.tee $0
                    (i32.shl
                     (local.get $4)
                     (local.get $0)
                    )
                   )
                   (i32.const 245760)
                  )
                  (i32.const 16)
                 )
                 (i32.const 2)
                )
               )
              )
             )
             (i32.shr_u
              (i32.shl
               (local.get $0)
               (local.get $1)
              )
              (i32.const 15)
             )
            )
           )
           (i32.const 1)
          )
          (i32.and
           (i32.shr_u
            (local.get $5)
            (i32.add
             (local.get $0)
             (i32.const 7)
            )
           )
           (i32.const 1)
          )
         )
        )
       )
       (i32.const 0)
      )
     )
     (i32.const 2)
    )
    (i32.const 1328)
   )
  )
  (i32.store offset=28
   (local.get $3)
   (local.get $1)
  )
  (i32.store offset=20
   (local.get $3)
   (i32.const 0)
  )
  (i32.store offset=16
   (local.get $3)
   (i32.const 0)
  )
  (if
   (i32.and
    (local.tee $4
     (i32.load
      (i32.const 1028)
     )
    )
    (local.tee $2
     (i32.shl
      (i32.const 1)
      (local.get $1)
     )
    )
   )
   (block $label$break$L197
    (if
     (i32.eq
      (local.get $5)
      (i32.and
       (i32.load offset=4
        (local.tee $0
         (i32.load
          (local.get $0)
         )
        )
       )
       (i32.const -8)
      )
     )
     (local.set $14
      (local.get $0)
     )
     (block $label$break$L200
      (local.set $4
       (i32.shl
        (local.get $5)
        (select
         (i32.const 0)
         (i32.sub
          (i32.const 25)
          (i32.shr_u
           (local.get $1)
           (i32.const 1)
          )
         )
         (i32.eq
          (local.get $1)
          (i32.const 31)
         )
        )
       )
      )
      (loop $while-in17
       (if
        (local.tee $1
         (i32.load
          (local.tee $2
           (i32.add
            (i32.add
             (local.get $0)
             (i32.const 16)
            )
            (i32.shl
             (i32.shr_u
              (local.get $4)
              (i32.const 31)
             )
             (i32.const 2)
            )
           )
          )
         )
        )
        (block
         (local.set $4
          (i32.shl
           (local.get $4)
           (i32.const 1)
          )
         )
         (if
          (i32.eq
           (local.get $5)
           (i32.and
            (i32.load offset=4
             (local.get $1)
            )
            (i32.const -8)
           )
          )
          (block
           (local.set $14
            (local.get $1)
           )
           (br $label$break$L200)
          )
          (block
           (local.set $0
            (local.get $1)
           )
           (br $while-in17)
          )
         )
        )
       )
      )
      (if
       (i32.gt_u
        (i32.load
         (i32.const 1040)
        )
        (local.get $2)
       )
       (call $_abort)
       (block
        (i32.store
         (local.get $2)
         (local.get $3)
        )
        (i32.store offset=24
         (local.get $3)
         (local.get $0)
        )
        (i32.store offset=12
         (local.get $3)
         (local.get $3)
        )
        (i32.store offset=8
         (local.get $3)
         (local.get $3)
        )
        (br $label$break$L197)
       )
      )
     )
    )
    (if
     (i32.and
      (i32.le_u
       (local.tee $0
        (i32.load
         (i32.const 1040)
        )
       )
       (local.get $14)
      )
      (i32.le_u
       (local.get $0)
       (local.tee $0
        (i32.load
         (local.tee $1
          (i32.add
           (local.get $14)
           (i32.const 8)
          )
         )
        )
       )
      )
     )
     (block
      (i32.store offset=12
       (local.get $0)
       (local.get $3)
      )
      (i32.store
       (local.get $1)
       (local.get $3)
      )
      (i32.store offset=8
       (local.get $3)
       (local.get $0)
      )
      (i32.store offset=12
       (local.get $3)
       (local.get $14)
      )
      (i32.store offset=24
       (local.get $3)
       (i32.const 0)
      )
     )
     (call $_abort)
    )
   )
   (block
    (i32.store
     (i32.const 1028)
     (i32.or
      (local.get $2)
      (local.get $4)
     )
    )
    (i32.store
     (local.get $0)
     (local.get $3)
    )
    (i32.store offset=24
     (local.get $3)
     (local.get $0)
    )
    (i32.store offset=12
     (local.get $3)
     (local.get $3)
    )
    (i32.store offset=8
     (local.get $3)
     (local.get $3)
    )
   )
  )
  (i32.store
   (i32.const 1056)
   (local.tee $0
    (i32.add
     (i32.load
      (i32.const 1056)
     )
     (i32.const -1)
    )
   )
  )
  (if
   (local.get $0)
   (return)
  )
  (local.set $0
   (i32.const 1480)
  )
  (loop $while-in19
   (local.set $0
    (i32.add
     (local.tee $3
      (i32.load
       (local.get $0)
      )
     )
     (i32.const 8)
    )
   )
   (br_if $while-in19
    (local.get $3)
   )
  )
  (i32.store
   (i32.const 1056)
   (i32.const -1)
  )
 )
 (func $___errno_location (; 14 ;) (; has Stack IR ;) (result i32)
  (i32.const 1520)
 )
 (func $_memcpy (; 15 ;) (; has Stack IR ;) (param $0 i32) (param $1 i32) (param $2 i32) (result i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (if
   (i32.ge_s
    (local.get $2)
    (i32.const 8192)
   )
   (block
    (drop
     (call $_emscripten_memcpy_big
      (local.get $0)
      (local.get $1)
      (local.get $2)
     )
    )
    (return
     (local.get $0)
    )
   )
  )
  (local.set $4
   (local.get $0)
  )
  (local.set $3
   (i32.add
    (local.get $0)
    (local.get $2)
   )
  )
  (if
   (i32.eq
    (i32.and
     (local.get $0)
     (i32.const 3)
    )
    (i32.and
     (local.get $1)
     (i32.const 3)
    )
   )
   (block
    (loop $while-in
     (if
      (i32.and
       (local.get $0)
       (i32.const 3)
      )
      (block
       (if
        (i32.eqz
         (local.get $2)
        )
        (return
         (local.get $4)
        )
       )
       (i32.store8
        (local.get $0)
        (i32.load8_s
         (local.get $1)
        )
       )
       (local.set $0
        (i32.add
         (local.get $0)
         (i32.const 1)
        )
       )
       (local.set $1
        (i32.add
         (local.get $1)
         (i32.const 1)
        )
       )
       (local.set $2
        (i32.sub
         (local.get $2)
         (i32.const 1)
        )
       )
       (br $while-in)
      )
     )
    )
    (local.set $5
     (i32.add
      (local.tee $2
       (i32.and
        (local.get $3)
        (i32.const -4)
       )
      )
      (i32.const -64)
     )
    )
    (loop $while-in1
     (if
      (i32.le_s
       (local.get $0)
       (local.get $5)
      )
      (block
       (i32.store
        (local.get $0)
        (i32.load
         (local.get $1)
        )
       )
       (i32.store offset=4
        (local.get $0)
        (i32.load offset=4
         (local.get $1)
        )
       )
       (i32.store offset=8
        (local.get $0)
        (i32.load offset=8
         (local.get $1)
        )
       )
       (i32.store offset=12
        (local.get $0)
        (i32.load offset=12
         (local.get $1)
        )
       )
       (i32.store offset=16
        (local.get $0)
        (i32.load offset=16
         (local.get $1)
        )
       )
       (i32.store offset=20
        (local.get $0)
        (i32.load offset=20
         (local.get $1)
        )
       )
       (i32.store offset=24
        (local.get $0)
        (i32.load offset=24
         (local.get $1)
        )
       )
       (i32.store offset=28
        (local.get $0)
        (i32.load offset=28
         (local.get $1)
        )
       )
       (i32.store offset=32
        (local.get $0)
        (i32.load offset=32
         (local.get $1)
        )
       )
       (i32.store offset=36
        (local.get $0)
        (i32.load offset=36
         (local.get $1)
        )
       )
       (i32.store offset=40
        (local.get $0)
        (i32.load offset=40
         (local.get $1)
        )
       )
       (i32.store offset=44
        (local.get $0)
        (i32.load offset=44
         (local.get $1)
        )
       )
       (i32.store offset=48
        (local.get $0)
        (i32.load offset=48
         (local.get $1)
        )
       )
       (i32.store offset=52
        (local.get $0)
        (i32.load offset=52
         (local.get $1)
        )
       )
       (i32.store offset=56
        (local.get $0)
        (i32.load offset=56
         (local.get $1)
        )
       )
       (i32.store offset=60
        (local.get $0)
        (i32.load offset=60
         (local.get $1)
        )
       )
       (local.set $0
        (i32.sub
         (local.get $0)
         (i32.const -64)
        )
       )
       (local.set $1
        (i32.sub
         (local.get $1)
         (i32.const -64)
        )
       )
       (br $while-in1)
      )
     )
    )
    (loop $while-in3
     (if
      (i32.lt_s
       (local.get $0)
       (local.get $2)
      )
      (block
       (i32.store
        (local.get $0)
        (i32.load
         (local.get $1)
        )
       )
       (local.set $0
        (i32.add
         (local.get $0)
         (i32.const 4)
        )
       )
       (local.set $1
        (i32.add
         (local.get $1)
         (i32.const 4)
        )
       )
       (br $while-in3)
      )
     )
    )
   )
   (block
    (local.set $2
     (i32.sub
      (local.get $3)
      (i32.const 4)
     )
    )
    (loop $while-in5
     (if
      (i32.lt_s
       (local.get $0)
       (local.get $2)
      )
      (block
       (i32.store8
        (local.get $0)
        (i32.load8_s
         (local.get $1)
        )
       )
       (i32.store8 offset=1
        (local.get $0)
        (i32.load8_s offset=1
         (local.get $1)
        )
       )
       (i32.store8 offset=2
        (local.get $0)
        (i32.load8_s offset=2
         (local.get $1)
        )
       )
       (i32.store8 offset=3
        (local.get $0)
        (i32.load8_s offset=3
         (local.get $1)
        )
       )
       (local.set $0
        (i32.add
         (local.get $0)
         (i32.const 4)
        )
       )
       (local.set $1
        (i32.add
         (local.get $1)
         (i32.const 4)
        )
       )
       (br $while-in5)
      )
     )
    )
   )
  )
  (loop $while-in7
   (if
    (i32.lt_s
     (local.get $0)
     (local.get $3)
    )
    (block
     (i32.store8
      (local.get $0)
      (i32.load8_s
       (local.get $1)
      )
     )
     (local.set $0
      (i32.add
       (local.get $0)
       (i32.const 1)
      )
     )
     (local.set $1
      (i32.add
       (local.get $1)
       (i32.const 1)
      )
     )
     (br $while-in7)
    )
   )
  )
  (local.get $4)
 )
 (func $_memset (; 16 ;) (; has Stack IR ;) (param $0 i32) (param $1 i32) (param $2 i32) (result i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (local.set $4
   (i32.add
    (local.get $0)
    (local.get $2)
   )
  )
  (local.set $1
   (i32.and
    (local.get $1)
    (i32.const 255)
   )
  )
  (if
   (i32.ge_s
    (local.get $2)
    (i32.const 67)
   )
   (block
    (loop $while-in
     (if
      (i32.and
       (local.get $0)
       (i32.const 3)
      )
      (block
       (i32.store8
        (local.get $0)
        (local.get $1)
       )
       (local.set $0
        (i32.add
         (local.get $0)
         (i32.const 1)
        )
       )
       (br $while-in)
      )
     )
    )
    (local.set $6
     (i32.add
      (local.tee $5
       (i32.and
        (local.get $4)
        (i32.const -4)
       )
      )
      (i32.const -64)
     )
    )
    (local.set $3
     (i32.or
      (i32.or
       (i32.or
        (i32.shl
         (local.get $1)
         (i32.const 8)
        )
        (local.get $1)
       )
       (i32.shl
        (local.get $1)
        (i32.const 16)
       )
      )
      (i32.shl
       (local.get $1)
       (i32.const 24)
      )
     )
    )
    (loop $while-in1
     (if
      (i32.le_s
       (local.get $0)
       (local.get $6)
      )
      (block
       (i32.store
        (local.get $0)
        (local.get $3)
       )
       (i32.store offset=4
        (local.get $0)
        (local.get $3)
       )
       (i32.store offset=8
        (local.get $0)
        (local.get $3)
       )
       (i32.store offset=12
        (local.get $0)
        (local.get $3)
       )
       (i32.store offset=16
        (local.get $0)
        (local.get $3)
       )
       (i32.store offset=20
        (local.get $0)
        (local.get $3)
       )
       (i32.store offset=24
        (local.get $0)
        (local.get $3)
       )
       (i32.store offset=28
        (local.get $0)
        (local.get $3)
       )
       (i32.store offset=32
        (local.get $0)
        (local.get $3)
       )
       (i32.store offset=36
        (local.get $0)
        (local.get $3)
       )
       (i32.store offset=40
        (local.get $0)
        (local.get $3)
       )
       (i32.store offset=44
        (local.get $0)
        (local.get $3)
       )
       (i32.store offset=48
        (local.get $0)
        (local.get $3)
       )
       (i32.store offset=52
        (local.get $0)
        (local.get $3)
       )
       (i32.store offset=56
        (local.get $0)
        (local.get $3)
       )
       (i32.store offset=60
        (local.get $0)
        (local.get $3)
       )
       (local.set $0
        (i32.sub
         (local.get $0)
         (i32.const -64)
        )
       )
       (br $while-in1)
      )
     )
    )
    (loop $while-in3
     (if
      (i32.lt_s
       (local.get $0)
       (local.get $5)
      )
      (block
       (i32.store
        (local.get $0)
        (local.get $3)
       )
       (local.set $0
        (i32.add
         (local.get $0)
         (i32.const 4)
        )
       )
       (br $while-in3)
      )
     )
    )
   )
  )
  (loop $while-in5
   (if
    (i32.lt_s
     (local.get $0)
     (local.get $4)
    )
    (block
     (i32.store8
      (local.get $0)
      (local.get $1)
     )
     (local.set $0
      (i32.add
       (local.get $0)
       (i32.const 1)
      )
     )
     (br $while-in5)
    )
   )
  )
  (i32.sub
   (local.get $4)
   (local.get $2)
  )
 )
 (func $_sbrk (; 17 ;) (; has Stack IR ;) (param $0 i32) (result i32)
  (local $1 i32)
  (local $2 i32)
  (if
   (i32.or
    (i32.and
     (i32.lt_s
      (local.tee $1
       (i32.add
        (local.get $0)
        (local.tee $2
         (i32.load
          (global.get $DYNAMICTOP_PTR)
         )
        )
       )
      )
      (local.get $2)
     )
     (i32.gt_s
      (local.get $0)
      (i32.const 0)
     )
    )
    (i32.lt_s
     (local.get $1)
     (i32.const 0)
    )
   )
   (block
    (drop
     (call $abortOnCannotGrowMemory)
    )
    (call $___setErrNo
     (i32.const 12)
    )
    (return
     (i32.const -1)
    )
   )
  )
  (if
   (i32.le_s
    (local.get $1)
    (call $_emscripten_get_heap_size)
   )
   (i32.store
    (global.get $DYNAMICTOP_PTR)
    (local.get $1)
   )
   (if
    (i32.eqz
     (call $_emscripten_resize_heap
      (local.get $1)
     )
    )
    (block
     (call $___setErrNo
      (i32.const 12)
     )
     (return
      (i32.const -1)
     )
    )
   )
  )
  (local.get $2)
 )
)
