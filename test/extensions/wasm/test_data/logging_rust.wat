(module
  (type $0 (func (param i32 i32)))
  (type $1 (func (param i32 i32 i32) (result i32)))
  (type $2 (func (param i32 i32) (result i32)))
  (type $3 (func (param i32 i32 i32)))
  (type $4 (func (param i32 i32 i32 i32 i32)))
  (type $5 (func (param i32)))
  (type $6 (func))
  (type $7 (func (param i32 i32 i32 i32)))
  (type $8 (func (param i32) (result i32)))
  (type $9 (func (param i64 i32) (result i32)))
  (type $10 (func (param i32) (result i64)))
  (type $11 (func (param i32 i32 i32 i32 i32) (result i32)))
  (type $12 (func (param i32 i32 i32 i32) (result i32)))
  (import "env" "_proxy_log" (func $_proxy_log (param i32 i32 i32) (result i32)))
  (export "memory" (memory $14))
  (export "__data_end" (global $16))
  (export "__heap_base" (global $17))
  (export "_proxy_onConfigure" (func $_proxy_onConfigure))
  (export "_proxy_onStart" (func $_proxy_onStart))
  (export "_proxy_onTick" (func $_proxy_onTick))
  (export "__post_instantiate" (func $__post_instantiate))
  (export "_malloc" (func $_malloc))
  (export "_free" (func $_free))
  (memory $14  17)
  (table $13  29 29 funcref)
  (global $15  (mut i32) (i32.const 1048576))
  (global $16  i32 (i32.const 1055168))
  (global $17  i32 (i32.const 1055168))
  (elem $18 $13 (i32.const 1)
    $<&T_as_core::fmt::Display>::fmt::h22df08b82f0869b0 $std::alloc::default_alloc_error_hook::ha6a42c7acd012d48 $core::fmt::ArgumentV1::show_usize::h42d8eed9ea762fba $<&T_as_core::fmt::Display>::fmt::h678bffb9cdc14530 $<&T_as_core::fmt::Debug>::fmt::h71574e0037640cf9 $core::fmt::num::imp::<impl_core::fmt::Display_for_u32>::fmt::h0a73e93540d1767b $<core::ops::range::Range<Idx>_as_core::fmt::Debug>::fmt::haf7bc23b95386435 $<char_as_core::fmt::Debug>::fmt::h27b79e1831e51870
    $core::ptr::real_drop_in_place::hb47e74d3b71496e8 $<&mut_W_as_core::fmt::Write>::write_str::h78ef2081843123b8 $<&mut_W_as_core::fmt::Write>::write_char::h4363fca537798ae0 $<&mut_W_as_core::fmt::Write>::write_fmt::hf8f3c5f5f4aa3d77 $core::ptr::real_drop_in_place::h38d0d042ad615819 $<proxy_wasm::Logger_as_log::Log>::enabled::hd59b56d376e99849 $<proxy_wasm::Logger_as_log::Log>::log::h69caf1c35822af17 $<log::NopLogger_as_log::Log>::flush::hf78c792c6fe83c70
    $<core::fmt::Error_as_core::fmt::Debug>::fmt::h99349d0cc7fe11e2 $<log::SetLoggerError_as_core::fmt::Debug>::fmt::h323d5f06fc0398c5 $<log::NopLogger_as_log::Log>::enabled::h4d0d6b60212bce89 $<log::NopLogger_as_log::Log>::log::hab97ea506f147727 $<T_as_core::any::Any>::type_id::h0651535bade0637a $core::ptr::real_drop_in_place::h4acead9551505031 $<core::fmt::builders::PadAdapter_as_core::fmt::Write>::write_str::h3cfa2330727f68d0 $core::fmt::Write::write_char::hddeb2d341a4f7c8e
    $core::fmt::Write::write_fmt::hdcf4af1ee4f15333 $<&mut_W_as_core::fmt::Write>::write_str::heb9faa6c9b7a53d6 $<&mut_W_as_core::fmt::Write>::write_char::h66c6e1685ede49cb $<&mut_W_as_core::fmt::Write>::write_fmt::hf64f7e3f726250ce)
  (data $19 $14 (i32.const 1048576)
    "warn \00\00\00\00\00\10\00\05\00\00\00logging_rustsrc/lib.rstest trace logging&\00\10\00\12\00\00\00"
    "test debug logging\00\00@\00\10\00\12\00\00\00test error logging\00\00\\\00\10\00\12\00\00\00test tic"
    "k logging\00\00\00x\00\10\00\11\00\00\00\09\00\00\00\04\00\00\00\04\00\00\00\n\00\00\00\0b\00\00\00\0c\00\00\00\c4\00\10\00$\00\00\00\f5\01\10\00\17\00\00\00D\02\00\00"
    "\09\00\00\00Tried to shrink to a larger capacity\0d\00\00\00\00\00\00\00\01\00\00\00\0e\00\00\00\0f\00\00\00\10\00\00\00"
    "\1c\02\10\00\00\00\00\00a Display implementation returned an error unexpectedly\00"
    "\0d\00\00\00\00\00\00\00\01\00\00\00\11\00\00\00called `Result::unwrap()` on an `Err` value\00\0d\00\00\00"
    "\00\00\00\00\01\00\00\00\12\00\00\00\0d\00\00\00\00\00\00\00\01\00\00\00\13\00\00\00\14\00\00\00\10\00\00\00\0d\00\00\00\00\00\00\00\01\00\00\00\13\00\00\00\14\00\00\00\10\00\00\00SetL"
    "oggerError()\e4\01\10\00\11\00\00\00\f5\01\10\00\17\00\00\00\f2\02\00\00\05\00\00\00capacity overflowsrc/liballo"
    "c/raw_vec.rs,\02\10\00 \00\00\00L\02\10\00\12\00\00\00\0d\00\00\00\00\00\00\00\01\00\00\00\15\00\00\00index out of bounds:"
    " the len is  but the index is 0001020304050607080910111213141516"
    "1718192021222324252627282930313233343536373839404142434445464748"
    "4950515253545556575859606162636465666768697071727374757677787980"
    "81828384858687888990919293949596979899\00\00`\03\10\00\06\00\00\00f\03\10\00\"\00\00\00H\03\10\00\18\00\00\00"
    "\17\n\00\00\05\00\00\00src/libcore/slice/mod.rsindex  out of range for slice of"
    " length \a8\03\10\00\16\00\00\00\be\03\10\00\0d\00\00\00H\03\10\00\18\00\00\00\1d\n\00\00\05\00\00\00slice index starts at  b"
    "ut ends at `0x[...]\00\92\04\10\00\0b\00\00\00B\17\10\00\16\00\00\00\cb\03\10\00\01\00\00\00|\04\10\00\16\00\00\00\03\08\00\00\09\00\00\00 \17\10\00"
    "\0e\00\00\00.\17\10\00\04\00\00\002\17\10\00\10\00\00\00\cb\03\10\00\01\00\00\00|\04\10\00\16\00\00\00\07\08\00\00\05\00\00\00\e0\16\10\00+\00\00\00\0b\17\10\00\15\00\00\00z\01\00\00"
    "\15\00\00\00\92\04\10\00\0b\00\00\00\9d\04\10\00&\00\00\00\c3\04\10\00\08\00\00\00\cb\04\10\00\06\00\00\00\cb\03\10\00\01\00\00\00|\04\10\00\16\00\00\00\14\08\00\00\05\00\00\00src/"
    "libcore/str/mod.rsbyte index  is not a char boundary; it is insi"
    "de  (bytes ) of `\00\00\00\16\05\10\00\02\00\00\00\00\05\10\00\16\00\00\00a\04\00\00\11\00\00\00\00\05\10\00\16\00\00\00U\04\00\00(\00\00\00\00\00\00\00"
    "src/libcore/fmt/mod.rs..\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\ff\ff\ff\ff\ff\ff\ff\ff"
    "\ff\ff\ff\ff\ff\ff\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\f8\03\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\fe\ff\ff\ff\ff\bf\b6\00\00\00\00\00\00\00\00\00\ff\07\00\00\00\00\00\f8\ff\ff\00\00\01\00\00\00\00\00\00\00\00\00\00\00\c0\9f\9f=\00\00\00\00\02\00\00\00\ff\ff"
    "\ff\07\00\00\00\00\00\00\00\00\00\00\c0\ff\01\00\00\00\00\00\00\f8\0f \c0\10\10\00J\00\00\00\10\13\10\00\00\02\00\00\10\15\10\00:\00\00\00\00\01\02\03\04\05\06\07\08\09\08\n\0b\0c\0d\0e"
    "\0f\10\11\12\13\14\02\15\16\17\18\19\1a\1b\1c\1d\1e\1f \02\02\02\02\02\02\02\02\02\02!\02\02\02\02\02\02\02\02\02\02\02\02\02\02\"#$%&\02'\02(\02\02\02)*+\02,-./"
    "0\02\021\02\02\022\02\02\02\02\02\02\02\023\02\024\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\025\026\027\02\02\02\02\02\02\02\028\029\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02:;<\02\02\02\02=\02\02>?@ABCDEF\02\02\02G"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02H\02\02\02"
    "\02\02\02\02\02\02\02\02I\02\02\02\02\02;\02\00\01\02\02\02\02\03\02\02\02\02\04\02\05\06\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\07\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\00\01\03\05\05\06\06\03\07\06\08\08\09\11\n\1c\0b\19\0c\14\0d\12\0e\0d\0f\04\10\03\12\12\13\09\16\01\17\05\18\02\19\03\1a\07\1c\02\1d\01\1f\16"
    " \03+\04,\02-\0b.\010\031\022\01\a7\02\a9\02\aa\04\ab\08\fa\02\fb\05\fd\04\fe\03\ff\09\adxy\8b\8d\a20WX\8b\8c\90\1c\1d\dd\0e\0fKL\fb\fc./?\\]_\b5\e2\84"
    "\8d\8e\91\92\a9\b1\ba\bb\c5\c6\c9\ca\de\e4\e5\ff\00\04\11\12)147:;=IJ]\84\8e\92\a9\b1\b4\ba\bb\c6\ca\ce\cf\e4\e5\00\04\0d\0e\11\12)14:;EFIJ^de\84\91"
    "\9b\9d\c9\ce\cf\0d\11)EIWde\8d\91\a9\b4\ba\bb\c5\c9\df\e4\e5\f0\04\0d\11EIde\80\81\84\b2\bc\be\bf\d5\d7\f0\f1\83\85\8b\a4\a6\be\bf\c5\c7\ce\cf\da\dbH\98\bd\cd\c6\ce\cfI"
    "NOWY^_\89\8e\8f\b1\b6\b7\bf\c1\c6\c7\d7\11\16\17[\\\f6\f7\fe\ff\80\0dmq\de\df\0e\0f\1fno\1c\1d_}~\ae\af\bb\bc\fa\16\17\1e\1fFGNOXZ\\^~\7f\b5\c5\d4"
    "\d5\dc\f0\f1\f5rs\8ftu\96\97/_&./\a7\af\b7\bf\c7\cf\d7\df\9a@\97\980\8f\1f\c0\c1\ce\ffNOZ[\07\08\0f\10'/\ee\efno7=?BE\90\91\fe\ffSgu\c8\c9"
    "\d0\d1\d8\d9\e7\fe\ff\00 _\"\82\df\04\82D\08\1b\04\06\11\81\ac\0e\80\ab5\1e\15\80\e0\03\19\08\01\04/\044\04\07\03\01\07\06\07\11\nP\0f\12\07U\08\02\04\1c\n\09\03\08\03\07\03"
    "\02\03\03\03\0c\04\05\03\0b\06\01\0e\15\05:\03\11\07\06\05\10\07W\07\02\07\15\0dP\04C\03-\03\01\04\11\06\0f\0c:\04\1d%_ m\04j%\80\c8\05\82\b0\03\1a\06\82\fd\03Y\07\15"
    "\0b\17\09\14\0c\14\0cj\06\n\06\1a\06Y\07+\05F\n,\04\0c\04\01\031\0b,\04\1a\06\0b\03\80\ac\06\n\06\1fAL\04-\03t\08<\03\0f\03<\078\08+\05\82\ff\11\18\08/\11-"
    "\03 \10!\0f\80\8c\04\82\97\19\0b\15\88\94\05/\05;\07\02\0e\18\09\80\b00t\0c\80\d6\1a\0c\05\80\ff\05\80\b6\05$\0c\9b\c6\n\d20\10\84\8d\037\09\81\\\14\80\b8\08\80\c705\04"
    "\n\068\08F\08\0c\06t\0b\1e\03Z\04Y\09\80\83\18\1c\n\16\09H\08\80\8a\06\ab\a4\0c\17\041\a1\04\81\da&\07\0c\05\05\80\a5\11\81m\10x(*\06L\04\80\8d\04\80\be\03\1b\03\0f"
    "\0d\00\06\01\01\03\01\04\02\08\08\09\02\n\05\0b\02\10\01\11\04\12\05\13\11\14\02\15\02\17\02\19\04\1c\05\1d\08$\01j\03k\02\bc\02\d1\02\d4\0c\d5\09\d6\02\d7\02\da\01\e0\05\e1\02\e8\02\ee"
    " \f0\04\f9\06\fa\02\0c';>NO\8f\9e\9e\9f\06\07\096=>V\f3\d0\d1\04\14\1867VW\bd5\ce\cf\e0\12\87\89\8e\9e\04\0d\0e\11\12)14:EFIJNOdeZ\\\b6"
    "\b7\1b\1c\a8\a9\d8\d9\097\90\91\a8\07\n;>fi\8f\92o_\ee\efZb\9a\9b'(U\9d\a0\a1\a3\a4\a7\a8\ad\ba\bc\c4\06\0b\0c\15\1d:?EQ\a6\a7\cc\cd\a0\07\19\1a\"%>?\c5"
    "\c6\04 #%&(38:HJLPSUVXZ\\^`cefksx}\7f\8a\a4\aa\af\b0\c0\d0\0cr\a3\a4\cb\ccno^\"{\05\03\04-\03e\04\01/.\80\82\1d\031\0f"
    "\1c\04$\09\1e\05+\05D\04\0e*\80\aa\06$\04$\04(\084\0b\01\80\90\817\09\16\n\08\80\989\03c\08\090\16\05!\03\1b\05\01@8\04K\05/\04\n\07\09\07@ '\04\0c\09"
    "6\03:\05\1a\07\04\0c\07PI73\0d3\07.\08\n\81&\1f\80\81(\08*\80\86\17\09N\04\1e\0fC\0e\19\07\n\06G\09'\09u\0b?A*\06;\05\n\06Q\06\01\05\10\03\05\80\8b"
    "` H\08\n\80\a6^\"E\0b\n\06\0d\139\07\n6,\04\10\80\c0<dS\0c\01\80\a0E\1bH\08S\1d9\81\07F\n\1d\03GI7\03\0e\08\n\069\07\n\816\19\80\c72\0d\83\9b"
    "fu\0b\80\c4\8a\bc\84/\8f\d1\82G\a1\b9\829\07*\04\02`&\nF\n(\05\13\82\b0[eK\049\07\11@\04\1c\97\f8\08\82\f3\a5\0d\81\1f1\03\11\04\08\81\8c\89\04k\05\0d\03\09"
    "\07\10\93`\80\f6\ns\08n\17F\80\9a\14\0cW\09\19\80\87\81G\03\85B\0f\15\85P+\80\d5-\03\1a\04\02\81p:\05\01\85\00\80\d7)L\04\n\04\02\83\11DL=\80\c2<\06\01\04"
    "U\05\1b4\02\81\0e,\04d\0cV\n\0d\03]\03=9\1d\0d,\04\09\07\02\0e\06\80\9a\83\d6\n\0d\03\0b\05t\0cY\07\0c\14\0c\048\08\n\06(\08\1eRw\031\03\80\a6\0c\14\04\03\05"
    "\03\0d\06\85j\00\00\00\a0\10\10\00 \00\00\00'\00\00\00\19\00\00\00\a0\10\10\00 \00\00\00(\00\00\00 \00\00\00\a0\10\10\00 \00\00\00*\00\00\00\19\00\00\00\a0\10\10\00 \00\00\00"
    "+\00\00\00\18\00\00\00\a0\10\10\00 \00\00\00,\00\00\00 \00\00\00\00\00\00\00\00\00\00\00src/libcore/unicode/bool_trie.rs"
    "\00\00\c0\fb\ef>\00\00\00\00\00\0e\00\00\00\00\00\00\00\00\00\00\00\00\00\00\f8\ff\fb\ff\ff\ff\07\00\00\00\00\00\00\14\fe!\fe\00\0c\00\00\00\02\00\00\00\00\00\00P\1e \80\00\0c\00\00@"
    "\06\00\00\00\00\00\00\10\869\02\00\00\00#\00\be!\00\00\0c\00\00\fc\02\00\00\00\00\00\00\d0\1e \c0\00\0c\00\00\00\04\00\00\00\00\00\00@\01 \80\00\00\00\00\00\11\00\00\00\00\00\00\c0"
    "\c1=`\00\0c\00\00\00\02\00\00\00\00\00\00\90D0`\00\0c\00\00\00\03\00\00\00\00\00\00X\1e \80\00\0c\00\00\00\00\84\\\80\00\00\00\00\00\00\00\00\00\00\f2\07\80\7f\00\00\00\00\00\00"
    "\00\00\00\00\00\00\f2\1f\00?\00\00\00\00\00\00\00\00\00\03\00\00\a0\02\00\00\00\00\00\00\fe\7f\df\e0\ff\fe\ff\ff\ff\1f@\00\00\00\00\00\00\00\00\00\00\00\00\e0\fdf\00\00\00\c3\01\00\1e\00"
    "d \00 \00\00\00\00\00\00\00\e0\00\00\00\00\00\00\1c\00\00\00\1c\00\00\00\0c\00\00\00\0c\00\00\00\00\00\00\00\b0?@\fe\0f \00\00\00\00\008\00\00\00\00\00\00`\00\00\00\00\02\00\00"
    "\00\00\00\00\87\01\04\0e\00\00\80\09\00\00\00\00\00\00@\7f\e5\1f\f8\9f\00\00\00\00\00\00\ff\7f\0f\00\00\00\00\00\f0\17\04\00\00\00\00\f8\0f\00\03\00\00\00<;\00\00\00\00\00\00@\a3\03\00"
    "\00\00\00\00\00\f0\cf\00\00\00\f7\ff\fd!\10\03\ff\ff\ff\ff\ff\ff\ff\fb\00\10\00\00\00\00\00\00\00\00\ff\ff\ff\ff\01\00\00\00\00\00\00\80\03\00\00\00\00\00\00\00\00\80\00\00\00\00\ff\ff\ff\ff"
    "\00\00\00\00\00\fc\00\00\00\00\00\06\00\00\00\00\00\00\00\00\00\80\f7?\00\00\00\c0\00\00\00\00\00\00\00\00\00\00\03\00D\08\00\00`\00\00\000\00\00\00\ff\ff\03\80\00\00\00\00\c0?\00\00"
    "\80\ff\03\00\00\00\00\00\07\00\00\00\00\00\c83\00\00\00\00 \00\00\00\00\00\00\00\00~f\00\08\10\00\00\00\00\00\10\00\00\00\00\00\00\9d\c1\02\00\00\00\000@\00\00\00\00\00 !\00\00"
    "\00\00\00@\00\00\00\00\ff\ff\00\00\ff\ff\00\00\00\00\00\00\00\00\00\01\00\00\00\02\00\03\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\04\00\00\05\00\00\00\00"
    "\00\00\00\00\06\00\00\00\00\00\00\00\00\07\00\00\08\09\n\00\0b\0c\0d\0e\0f\00\00\10\11\12\00\00\13\14\15\16\00\00\17\18\19\1a\1b\00\1c\00\00\00\1d\00\00\00\00\00\00\1e\1f !\00\00\00\00\00"
    "\"\00#\00$%&\00\00\00\00'\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00()\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00*+\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00,\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00-.\00\00/\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00012\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\003\00\00\00)\00\00\00\00\00\004\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\005\006\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\0078\00\008889\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00 \00\00\00\00\01\00\00\00\00\00\00\00\00\00\c0\07n\f0\00\00\00\00\00\87\00\00\00\00`\00\00\00"
    "\00\00\00\00\f0\00\00\00\c0\ff\01\00\00\00\00\00\02\00\00\00\00\00\00\ff\7f\00\00\00\00\00\00\80\03\00\00\00\00\00x\06\07\00\00\00\80\ef\1f\00\00\00\00\00\00\00\08\00\03\00\00\00\00\00\c0\7f"
    "\00\1e\00\00\00\00\00\00\00\00\00\00\00\80\d3@\00\00\00\80\f8\07\00\00\03\00\00\00\00\00\00X\01\00\80\00\c0\1f\1f\00\00\00\00\00\00\00\00\ff\\\00\00@\00\00\00\00\00\00\00\00\00\00\f9\a5"
    "\0d\00\00\00\00\00\00\00\00\00\00\00\00\80<\b0\01\00\000\00\00\00\00\00\00\00\00\00\00\f8\a7\01\00\00\00\00\00\00\00\00\00\00\00\00(\bf\00\00\00\00\e0\bc\0f\00\00\00\00\00\00\00\80\ff\06"
    "\00\00\f0\0c\01\00\00\00\fe\07\00\00\00\00\f8y\80\00~\0e\00\00\00\00\00\fc\7f\03\00\00\00\00\00\00\00\00\00\00\7f\bf\00\00\fc\ff\ff\fcm\00\00\00\00\00\00\00~\b4\bf\00\00\00\00\00\00\00"
    "\00\00\a3\00\00\00\00\00\00\00\00\00\00\00\18\00\00\00\00\00\00\00\1f\00\00\00\00\00\00\00\7f\00\00\80\00\00\00\00\00\00\00\80\07\00\00\00\00\00\00\00\00`\00\00\00\00\00\00\00\00\a0\c3\07\f8"
    "\e7\0f\00\00\00<\00\00\1c\00\00\00\00\00\00\00\ff\ff\ff\ff\ff\ff\7f\f8\ff\ff\ff\ff\ff\1f \00\10\00\00\f8\fe\ff\00\00\7f\ff\ff\f9\db\07\00\00\00\00\00\00\00\f0\00\00\00\00\7f\00\00\00\00\00"
    "\f0\07\00\00\00\00\00\00\00\00\00\00\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\00\00called `Option::unwrap()` on a `"
    "None` valuesrc/libcore/option.rsbegin <= end ( <= ) when slicing"
    " ` is out of bounds of `\1c\02\10\00\00\00\00\00h\17\10\00\02\00\00\00: \00\00|\17\10\00\15\00\00\00<\04\00\00\05\00\00\00src/"
    "libcore/result.rs)\00\00\16\00\00\00\0c\00\00\00\04\00\00\00\17\00\00\00\18\00\00\00\19\00\00\00,\n\00\00\09\00\00\00\04\00\00\00\04\00\00\00\1a\00\00\00"
    "\1b\00\00\00\1c\00\00\00    ((\nError")
  (data $20 $14 (i32.const 1054680)
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00")
  (data $21 $14 (i32.const 1055160)
    "\e0\17\10\00\8c\01\10\00")
  
  (func $_proxy_onConfigure (type $3)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (local $3 i32)
    (local $4 i32)
    global.get $15
    i32.const 80
    i32.sub
    local.tee $3
    global.set $15
    local.get $3
    local.get $2
    i32.store offset=4
    local.get $3
    local.get $1
    i32.store
    block $block
      i32.const 0
      i32.load offset=1054680
      i32.const 2
      i32.lt_u
      br_if $block
      local.get $3
      i32.const 1
      i32.store offset=12
      local.get $3
      local.get $3
      i32.store offset=8
      i32.const 0
      i32.load offset=1055164
      local.set $1
      i32.const 0
      i32.load offset=1055160
      local.set $2
      i32.const 0
      i32.load offset=1054684
      local.set $4
      local.get $3
      i32.const 72
      i32.add
      i32.const 10
      i32.store
      local.get $3
      i32.const 64
      i32.add
      i64.const 4294967306
      i64.store
      local.get $3
      i32.const 56
      i32.add
      i32.const 12
      i32.store
      local.get $3
      i32.const 48
      i32.add
      i32.const 1
      i32.store
      local.get $3
      i32.const 32
      i32.add
      i64.const 1
      i64.store
      local.get $3
      i32.const 24
      i32.add
      i32.const 12
      i32.store
      local.get $3
      i32.const 1048604
      i32.store offset=60
      local.get $3
      i32.const 1048592
      i32.store offset=52
      local.get $3
      i32.const 44
      i32.add
      local.get $3
      i32.const 8
      i32.add
      i32.store
      local.get $3
      i32.const 1048584
      i32.store offset=28
      local.get $3
      i32.const 1048592
      i32.store offset=20
      local.get $3
      i32.const 2
      i32.store offset=16
      local.get $2
      i32.const 1048972
      local.get $4
      i32.const 2
      i32.eq
      local.tee $4
      select
      local.get $3
      i32.const 16
      i32.add
      local.get $1
      i32.const 1048996
      local.get $4
      select
      i32.load offset=16
      call_indirect $13 (type $0)
    end ;; $block
    local.get $3
    i32.const 80
    i32.add
    global.set $15
    )
  
  (func $<&T_as_core::fmt::Display>::fmt::h22df08b82f0869b0 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    local.get $1
    local.get $0
    i32.load
    local.get $0
    i32.load offset=4
    call $core::fmt::Formatter::pad::h65a2184638dd238f
    )
  
  (func $_proxy_onStart (type $4)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (param $4 i32)
    (local $5 i32)
    (local $6 i32)
    (local $7 i32)
    (local $8 i32)
    global.get $15
    i32.const 64
    i32.sub
    local.tee $5
    global.set $15
    block $block
      i32.const 0
      i32.load offset=1054680
      i32.const 5
      i32.lt_u
      br_if $block
      i32.const 0
      i32.load offset=1055164
      local.set $6
      i32.const 0
      i32.load offset=1055160
      local.set $7
      i32.const 0
      i32.load offset=1054684
      local.set $8
      local.get $5
      i32.const 56
      i32.add
      i32.const 16
      i32.store
      local.get $5
      i32.const 48
      i32.add
      i64.const 4294967306
      i64.store
      local.get $5
      i32.const 40
      i32.add
      i32.const 12
      i32.store
      local.get $5
      i32.const 32
      i32.add
      i32.const 0
      i32.store
      local.get $5
      i32.const 28
      i32.add
      i32.const 1049116
      i32.store
      local.get $5
      i32.const 16
      i32.add
      i64.const 1
      i64.store
      local.get $5
      i32.const 8
      i32.add
      i32.const 12
      i32.store
      local.get $5
      i32.const 1048604
      i32.store offset=44
      local.get $5
      i32.const 1048592
      i32.store offset=36
      local.get $5
      i32.const 1048632
      i32.store offset=12
      local.get $5
      i32.const 1048592
      i32.store offset=4
      local.get $5
      i32.const 5
      i32.store
      local.get $7
      i32.const 1048972
      local.get $8
      i32.const 2
      i32.eq
      local.tee $8
      select
      local.get $5
      local.get $6
      i32.const 1048996
      local.get $8
      select
      i32.load offset=16
      call_indirect $13 (type $0)
    end ;; $block
    block $block_0
      i32.const 0
      i32.load offset=1054680
      i32.const 4
      i32.lt_u
      br_if $block_0
      i32.const 0
      i32.load offset=1055164
      local.set $6
      i32.const 0
      i32.load offset=1055160
      local.set $7
      i32.const 0
      i32.load offset=1054684
      local.set $8
      local.get $5
      i32.const 56
      i32.add
      i32.const 17
      i32.store
      local.get $5
      i32.const 48
      i32.add
      i64.const 4294967306
      i64.store
      local.get $5
      i32.const 40
      i32.add
      i32.const 12
      i32.store
      local.get $5
      i32.const 32
      i32.add
      i32.const 0
      i32.store
      local.get $5
      i32.const 28
      i32.add
      i32.const 1049116
      i32.store
      local.get $5
      i32.const 16
      i32.add
      i64.const 1
      i64.store
      local.get $5
      i32.const 8
      i32.add
      i32.const 12
      i32.store
      local.get $5
      i32.const 1048604
      i32.store offset=44
      local.get $5
      i32.const 1048592
      i32.store offset=36
      local.get $5
      i32.const 1048660
      i32.store offset=12
      local.get $5
      i32.const 1048592
      i32.store offset=4
      local.get $5
      i32.const 4
      i32.store
      local.get $7
      i32.const 1048972
      local.get $8
      i32.const 2
      i32.eq
      local.tee $8
      select
      local.get $5
      local.get $6
      i32.const 1048996
      local.get $8
      select
      i32.load offset=16
      call_indirect $13 (type $0)
    end ;; $block_0
    block $block_1
      i32.const 0
      i32.load offset=1054680
      i32.eqz
      br_if $block_1
      i32.const 0
      i32.load offset=1055164
      local.set $6
      i32.const 0
      i32.load offset=1055160
      local.set $7
      i32.const 0
      i32.load offset=1054684
      local.set $8
      local.get $5
      i32.const 56
      i32.add
      i32.const 18
      i32.store
      local.get $5
      i32.const 48
      i32.add
      i64.const 4294967306
      i64.store
      local.get $5
      i32.const 40
      i32.add
      i32.const 12
      i32.store
      local.get $5
      i32.const 32
      i32.add
      i32.const 0
      i32.store
      local.get $5
      i32.const 28
      i32.add
      i32.const 1049116
      i32.store
      local.get $5
      i32.const 16
      i32.add
      i64.const 1
      i64.store
      local.get $5
      i32.const 8
      i32.add
      i32.const 12
      i32.store
      local.get $5
      i32.const 1048604
      i32.store offset=44
      local.get $5
      i32.const 1048592
      i32.store offset=36
      local.get $5
      i32.const 1048688
      i32.store offset=12
      local.get $5
      i32.const 1048592
      i32.store offset=4
      local.get $5
      i32.const 1
      i32.store
      local.get $7
      i32.const 1048972
      local.get $8
      i32.const 2
      i32.eq
      local.tee $8
      select
      local.get $5
      local.get $6
      i32.const 1048996
      local.get $8
      select
      i32.load offset=16
      call_indirect $13 (type $0)
    end ;; $block_1
    local.get $5
    i32.const 64
    i32.add
    global.set $15
    )
  
  (func $_proxy_onTick (type $5)
    (param $0 i32)
    (local $1 i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    global.get $15
    i32.const 64
    i32.sub
    local.tee $1
    global.set $15
    block $block
      i32.const 0
      i32.load offset=1054680
      i32.const 3
      i32.lt_u
      br_if $block
      i32.const 0
      i32.load offset=1055164
      local.set $2
      i32.const 0
      i32.load offset=1055160
      local.set $3
      i32.const 0
      i32.load offset=1054684
      local.set $4
      local.get $1
      i32.const 56
      i32.add
      i32.const 24
      i32.store
      local.get $1
      i32.const 48
      i32.add
      i64.const 4294967306
      i64.store
      local.get $1
      i32.const 40
      i32.add
      i32.const 12
      i32.store
      local.get $1
      i32.const 32
      i32.add
      i32.const 0
      i32.store
      local.get $1
      i32.const 28
      i32.add
      i32.const 1049116
      i32.store
      local.get $1
      i32.const 16
      i32.add
      i64.const 1
      i64.store
      local.get $1
      i32.const 8
      i32.add
      i32.const 12
      i32.store
      local.get $1
      i32.const 1048604
      i32.store offset=44
      local.get $1
      i32.const 1048592
      i32.store offset=36
      local.get $1
      i32.const 1048716
      i32.store offset=12
      local.get $1
      i32.const 1048592
      i32.store offset=4
      local.get $1
      i32.const 3
      i32.store
      local.get $3
      i32.const 1048972
      local.get $4
      i32.const 2
      i32.eq
      local.tee $4
      select
      local.get $1
      local.get $2
      i32.const 1048996
      local.get $4
      select
      i32.load offset=16
      call_indirect $13 (type $0)
    end ;; $block
    local.get $1
    i32.const 64
    i32.add
    global.set $15
    )
  
  (func $core::ptr::real_drop_in_place::hb47e74d3b71496e8 (type $5)
    (param $0 i32)
    )
  
  (func $<&mut_W_as_core::fmt::Write>::write_str::h78ef2081843123b8 (type $1)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    block $block
      block $block_0
        block $block_1
          block $block_2
            local.get $0
            i32.load
            local.tee $0
            i32.const 4
            i32.add
            i32.load
            local.tee $3
            local.get $0
            i32.const 8
            i32.add
            i32.load
            local.tee $4
            i32.sub
            local.get $2
            i32.lt_u
            br_if $block_2
            local.get $0
            i32.load
            local.set $3
            br $block_1
          end ;; $block_2
          local.get $4
          local.get $2
          i32.add
          local.tee $5
          local.get $4
          i32.lt_u
          br_if $block
          local.get $3
          i32.const 1
          i32.shl
          local.tee $4
          local.get $5
          local.get $5
          local.get $4
          i32.lt_u
          select
          local.tee $4
          i32.const 0
          i32.lt_s
          br_if $block
          block $block_3
            block $block_4
              local.get $3
              br_if $block_4
              local.get $4
              i32.const 1
              call $__rust_alloc
              local.set $3
              br $block_3
            end ;; $block_4
            local.get $0
            i32.load
            local.get $4
            call $__rust_realloc
            local.set $3
          end ;; $block_3
          local.get $3
          i32.eqz
          br_if $block_0
          local.get $0
          local.get $3
          i32.store
          local.get $0
          i32.const 4
          i32.add
          local.get $4
          i32.store
          local.get $0
          i32.const 8
          i32.add
          i32.load
          local.set $4
        end ;; $block_1
        local.get $0
        i32.const 8
        i32.add
        local.get $4
        local.get $2
        i32.add
        i32.store
        local.get $3
        local.get $4
        i32.add
        local.get $1
        local.get $2
        call $memcpy
        drop
        i32.const 0
        return
      end ;; $block_0
      local.get $4
      i32.const 1
      i32.const 0
      i32.load offset=1055156
      local.tee $0
      i32.const 2
      local.get $0
      select
      call_indirect $13 (type $0)
      unreachable
      unreachable
    end ;; $block
    call $alloc::raw_vec::capacity_overflow::h8b890ddaa46c20bf
    unreachable
    )
  
  (func $__rust_alloc (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    local.get $0
    local.get $1
    call $__rdl_alloc
    )
  
  (func $__rust_realloc (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    local.get $0
    local.get $1
    call $__rdl_realloc
    )
  
  (func $std::alloc::default_alloc_error_hook::ha6a42c7acd012d48 (type $0)
    (param $0 i32)
    (param $1 i32)
    )
  
  (func $alloc::raw_vec::capacity_overflow::h8b890ddaa46c20bf (type $6)
    i32.const 1049036
    call $core::panicking::panic::h540eb5ee6ff533c2
    unreachable
    )
  
  (func $<&mut_W_as_core::fmt::Write>::write_char::h4363fca537798ae0 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    global.get $15
    i32.const 16
    i32.sub
    local.tee $2
    global.set $15
    local.get $0
    i32.load
    local.set $0
    block $block
      block $block_0
        block $block_1
          block $block_2
            block $block_3
              local.get $1
              i32.const 128
              i32.lt_u
              br_if $block_3
              local.get $2
              i32.const 0
              i32.store offset=12
              block $block_4
                block $block_5
                  local.get $1
                  i32.const 2047
                  i32.gt_u
                  br_if $block_5
                  local.get $2
                  local.get $1
                  i32.const 63
                  i32.and
                  i32.const 128
                  i32.or
                  i32.store8 offset=13
                  local.get $2
                  local.get $1
                  i32.const 6
                  i32.shr_u
                  i32.const 31
                  i32.and
                  i32.const 192
                  i32.or
                  i32.store8 offset=12
                  i32.const 2
                  local.set $1
                  br $block_4
                end ;; $block_5
                block $block_6
                  local.get $1
                  i32.const 65535
                  i32.gt_u
                  br_if $block_6
                  local.get $2
                  local.get $1
                  i32.const 63
                  i32.and
                  i32.const 128
                  i32.or
                  i32.store8 offset=14
                  local.get $2
                  local.get $1
                  i32.const 6
                  i32.shr_u
                  i32.const 63
                  i32.and
                  i32.const 128
                  i32.or
                  i32.store8 offset=13
                  local.get $2
                  local.get $1
                  i32.const 12
                  i32.shr_u
                  i32.const 15
                  i32.and
                  i32.const 224
                  i32.or
                  i32.store8 offset=12
                  i32.const 3
                  local.set $1
                  br $block_4
                end ;; $block_6
                local.get $2
                local.get $1
                i32.const 63
                i32.and
                i32.const 128
                i32.or
                i32.store8 offset=15
                local.get $2
                local.get $1
                i32.const 18
                i32.shr_u
                i32.const 240
                i32.or
                i32.store8 offset=12
                local.get $2
                local.get $1
                i32.const 6
                i32.shr_u
                i32.const 63
                i32.and
                i32.const 128
                i32.or
                i32.store8 offset=14
                local.get $2
                local.get $1
                i32.const 12
                i32.shr_u
                i32.const 63
                i32.and
                i32.const 128
                i32.or
                i32.store8 offset=13
                i32.const 4
                local.set $1
              end ;; $block_4
              block $block_7
                block $block_8
                  local.get $0
                  i32.const 4
                  i32.add
                  i32.load
                  local.tee $3
                  local.get $0
                  i32.const 8
                  i32.add
                  i32.load
                  local.tee $4
                  i32.sub
                  local.get $1
                  i32.lt_u
                  br_if $block_8
                  local.get $0
                  i32.load
                  local.set $3
                  br $block_7
                end ;; $block_8
                local.get $4
                local.get $1
                i32.add
                local.tee $5
                local.get $4
                i32.lt_u
                br_if $block
                local.get $3
                i32.const 1
                i32.shl
                local.tee $4
                local.get $5
                local.get $5
                local.get $4
                i32.lt_u
                select
                local.tee $4
                i32.const 0
                i32.lt_s
                br_if $block
                block $block_9
                  block $block_10
                    local.get $3
                    br_if $block_10
                    local.get $4
                    i32.const 1
                    call $__rust_alloc
                    local.set $3
                    br $block_9
                  end ;; $block_10
                  local.get $0
                  i32.load
                  local.get $4
                  call $__rust_realloc
                  local.set $3
                end ;; $block_9
                local.get $3
                i32.eqz
                br_if $block_1
                local.get $0
                local.get $3
                i32.store
                local.get $0
                i32.const 4
                i32.add
                local.get $4
                i32.store
                local.get $0
                i32.const 8
                i32.add
                i32.load
                local.set $4
              end ;; $block_7
              local.get $0
              i32.const 8
              i32.add
              local.get $4
              local.get $1
              i32.add
              i32.store
              local.get $3
              local.get $4
              i32.add
              local.get $2
              i32.const 12
              i32.add
              local.get $1
              call $memcpy
              drop
              br $block_2
            end ;; $block_3
            block $block_11
              local.get $0
              i32.load offset=8
              local.tee $4
              local.get $0
              i32.const 4
              i32.add
              i32.load
              i32.ne
              br_if $block_11
              local.get $4
              i32.const 1
              i32.add
              local.tee $3
              local.get $4
              i32.lt_u
              br_if $block
              local.get $4
              i32.const 1
              i32.shl
              local.tee $5
              local.get $3
              local.get $3
              local.get $5
              i32.lt_u
              select
              local.tee $3
              i32.const 0
              i32.lt_s
              br_if $block
              block $block_12
                block $block_13
                  local.get $4
                  br_if $block_13
                  local.get $3
                  i32.const 1
                  call $__rust_alloc
                  local.set $4
                  br $block_12
                end ;; $block_13
                local.get $0
                i32.load
                local.get $3
                call $__rust_realloc
                local.set $4
              end ;; $block_12
              local.get $4
              i32.eqz
              br_if $block_0
              local.get $0
              local.get $4
              i32.store
              local.get $0
              i32.const 4
              i32.add
              local.get $3
              i32.store
              local.get $0
              i32.load offset=8
              local.set $4
            end ;; $block_11
            local.get $0
            i32.load
            local.get $4
            i32.add
            local.get $1
            i32.store8
            local.get $0
            local.get $0
            i32.load offset=8
            i32.const 1
            i32.add
            i32.store offset=8
          end ;; $block_2
          local.get $2
          i32.const 16
          i32.add
          global.set $15
          i32.const 0
          return
        end ;; $block_1
        local.get $4
        i32.const 1
        i32.const 0
        i32.load offset=1055156
        local.tee $0
        i32.const 2
        local.get $0
        select
        call_indirect $13 (type $0)
        unreachable
        unreachable
      end ;; $block_0
      local.get $3
      i32.const 1
      i32.const 0
      i32.load offset=1055156
      local.tee $0
      i32.const 2
      local.get $0
      select
      call_indirect $13 (type $0)
      unreachable
      unreachable
    end ;; $block
    call $alloc::raw_vec::capacity_overflow::h8b890ddaa46c20bf
    unreachable
    )
  
  (func $<&mut_W_as_core::fmt::Write>::write_fmt::hf8f3c5f5f4aa3d77 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    global.get $15
    i32.const 32
    i32.sub
    local.tee $2
    global.set $15
    local.get $2
    local.get $0
    i32.load
    i32.store offset=4
    local.get $2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get $1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $2
    local.get $1
    i64.load align=4
    i64.store offset=8
    local.get $2
    i32.const 4
    i32.add
    i32.const 1048724
    local.get $2
    i32.const 8
    i32.add
    call $core::fmt::write::h9ffb25a5a03fc281
    local.set $1
    local.get $2
    i32.const 32
    i32.add
    global.set $15
    local.get $1
    )
  
  (func $core::fmt::write::h9ffb25a5a03fc281 (type $1)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    (local $7 i32)
    (local $8 i32)
    (local $9 i32)
    (local $10 i32)
    global.get $15
    i32.const 64
    i32.sub
    local.tee $3
    global.set $15
    local.get $3
    i32.const 36
    i32.add
    local.get $1
    i32.store
    local.get $3
    i32.const 52
    i32.add
    local.get $2
    i32.const 20
    i32.add
    i32.load
    local.tee $4
    i32.store
    local.get $3
    i32.const 3
    i32.store8 offset=56
    local.get $3
    i32.const 44
    i32.add
    local.get $2
    i32.load offset=16
    local.tee $5
    local.get $4
    i32.const 3
    i32.shl
    i32.add
    i32.store
    local.get $3
    i64.const 137438953472
    i64.store offset=8
    local.get $3
    local.get $0
    i32.store offset=32
    i32.const 0
    local.set $6
    local.get $3
    i32.const 0
    i32.store offset=24
    local.get $3
    i32.const 0
    i32.store offset=16
    local.get $3
    local.get $5
    i32.store offset=48
    local.get $3
    local.get $5
    i32.store offset=40
    block $block
      block $block_0
        block $block_1
          block $block_2
            block $block_3
              local.get $2
              i32.load offset=8
              local.tee $7
              br_if $block_3
              local.get $2
              i32.load
              local.set $8
              local.get $2
              i32.load offset=4
              local.tee $9
              local.get $4
              local.get $4
              local.get $9
              i32.gt_u
              select
              local.tee $10
              i32.eqz
              br_if $block_2
              i32.const 1
              local.set $4
              local.get $0
              local.get $8
              i32.load
              local.get $8
              i32.load offset=4
              local.get $1
              i32.load offset=12
              call_indirect $13 (type $1)
              br_if $block
              local.get $8
              i32.const 8
              i32.add
              local.set $2
              i32.const 1
              local.set $6
              loop $loop
                block $block_4
                  local.get $5
                  i32.load
                  local.get $3
                  i32.const 8
                  i32.add
                  local.get $5
                  i32.const 4
                  i32.add
                  i32.load
                  call_indirect $13 (type $2)
                  i32.eqz
                  br_if $block_4
                  i32.const 1
                  local.set $4
                  br $block
                end ;; $block_4
                local.get $6
                local.get $10
                i32.ge_u
                br_if $block_2
                local.get $2
                i32.const 4
                i32.add
                local.set $0
                local.get $2
                i32.load
                local.set $1
                local.get $5
                i32.const 8
                i32.add
                local.set $5
                local.get $2
                i32.const 8
                i32.add
                local.set $2
                i32.const 1
                local.set $4
                local.get $6
                i32.const 1
                i32.add
                local.set $6
                local.get $3
                i32.load offset=32
                local.get $1
                local.get $0
                i32.load
                local.get $3
                i32.load offset=36
                i32.load offset=12
                call_indirect $13 (type $1)
                i32.eqz
                br_if $loop
                br $block
              end ;; $loop
            end ;; $block_3
            local.get $2
            i32.load
            local.set $8
            local.get $2
            i32.load offset=4
            local.tee $9
            local.get $2
            i32.const 12
            i32.add
            i32.load
            local.tee $5
            local.get $5
            local.get $9
            i32.gt_u
            select
            local.tee $10
            i32.eqz
            br_if $block_2
            i32.const 1
            local.set $4
            local.get $0
            local.get $8
            i32.load
            local.get $8
            i32.load offset=4
            local.get $1
            i32.load offset=12
            call_indirect $13 (type $1)
            br_if $block
            local.get $7
            i32.const 16
            i32.add
            local.set $5
            local.get $8
            i32.const 8
            i32.add
            local.set $2
            i32.const 1
            local.set $6
            loop $loop_0
              local.get $3
              local.get $5
              i32.const -8
              i32.add
              i32.load
              i32.store offset=12
              local.get $3
              local.get $5
              i32.const 16
              i32.add
              i32.load8_u
              i32.store8 offset=56
              local.get $3
              local.get $5
              i32.const -4
              i32.add
              i32.load
              i32.store offset=8
              i32.const 0
              local.set $1
              i32.const 0
              local.set $4
              block $block_5
                block $block_6
                  block $block_7
                    block $block_8
                      local.get $5
                      i32.const 8
                      i32.add
                      i32.load
                      br_table
                        $block_8 $block_7 $block_6 $block_5
                        $block_8 ;; default
                    end ;; $block_8
                    local.get $5
                    i32.const 12
                    i32.add
                    i32.load
                    local.set $0
                    i32.const 1
                    local.set $4
                    br $block_5
                  end ;; $block_7
                  block $block_9
                    local.get $5
                    i32.const 12
                    i32.add
                    i32.load
                    local.tee $7
                    local.get $3
                    i32.load offset=52
                    local.tee $4
                    i32.ge_u
                    br_if $block_9
                    i32.const 0
                    local.set $4
                    local.get $3
                    i32.load offset=48
                    local.get $7
                    i32.const 3
                    i32.shl
                    i32.add
                    local.tee $7
                    i32.load offset=4
                    i32.const 3
                    i32.ne
                    br_if $block_5
                    local.get $7
                    i32.load
                    i32.load
                    local.set $0
                    i32.const 1
                    local.set $4
                    br $block_5
                  end ;; $block_9
                  i32.const 1049820
                  local.get $7
                  local.get $4
                  call $core::panicking::panic_bounds_check::hb65bbcdf415380e4
                  unreachable
                end ;; $block_6
                i32.const 0
                local.set $4
                local.get $3
                i32.load offset=40
                local.tee $7
                local.get $3
                i32.load offset=44
                i32.eq
                br_if $block_5
                local.get $3
                local.get $7
                i32.const 8
                i32.add
                i32.store offset=40
                i32.const 0
                local.set $4
                local.get $7
                i32.load offset=4
                i32.const 3
                i32.ne
                br_if $block_5
                local.get $7
                i32.load
                i32.load
                local.set $0
                i32.const 1
                local.set $4
              end ;; $block_5
              local.get $3
              local.get $0
              i32.store offset=20
              local.get $3
              local.get $4
              i32.store offset=16
              block $block_10
                block $block_11
                  block $block_12
                    block $block_13
                      block $block_14
                        block $block_15
                          block $block_16
                            local.get $5
                            i32.load
                            br_table
                              $block_12 $block_15 $block_16 $block_10
                              $block_12 ;; default
                          end ;; $block_16
                          local.get $3
                          i32.load offset=40
                          local.tee $0
                          local.get $3
                          i32.load offset=44
                          i32.ne
                          br_if $block_14
                          br $block_10
                        end ;; $block_15
                        local.get $5
                        i32.const 4
                        i32.add
                        i32.load
                        local.tee $0
                        local.get $3
                        i32.load offset=52
                        local.tee $4
                        i32.ge_u
                        br_if $block_13
                        local.get $3
                        i32.load offset=48
                        local.get $0
                        i32.const 3
                        i32.shl
                        i32.add
                        local.tee $0
                        i32.load offset=4
                        i32.const 3
                        i32.ne
                        br_if $block_10
                        local.get $0
                        i32.load
                        i32.load
                        local.set $4
                        br $block_11
                      end ;; $block_14
                      local.get $3
                      local.get $0
                      i32.const 8
                      i32.add
                      i32.store offset=40
                      local.get $0
                      i32.load offset=4
                      i32.const 3
                      i32.ne
                      br_if $block_10
                      local.get $0
                      i32.load
                      i32.load
                      local.set $4
                      br $block_11
                    end ;; $block_13
                    i32.const 1049820
                    local.get $0
                    local.get $4
                    call $core::panicking::panic_bounds_check::hb65bbcdf415380e4
                    unreachable
                  end ;; $block_12
                  local.get $5
                  i32.const 4
                  i32.add
                  i32.load
                  local.set $4
                end ;; $block_11
                i32.const 1
                local.set $1
              end ;; $block_10
              local.get $3
              local.get $4
              i32.store offset=28
              local.get $3
              local.get $1
              i32.store offset=24
              block $block_17
                block $block_18
                  local.get $5
                  i32.const -16
                  i32.add
                  i32.load
                  i32.const 1
                  i32.eq
                  br_if $block_18
                  local.get $3
                  i32.load offset=40
                  local.tee $4
                  local.get $3
                  i32.load offset=44
                  i32.eq
                  br_if $block_1
                  local.get $3
                  local.get $4
                  i32.const 8
                  i32.add
                  i32.store offset=40
                  br $block_17
                end ;; $block_18
                local.get $5
                i32.const -12
                i32.add
                i32.load
                local.tee $4
                local.get $3
                i32.load offset=52
                local.tee $0
                i32.ge_u
                br_if $block_0
                local.get $3
                i32.load offset=48
                local.get $4
                i32.const 3
                i32.shl
                i32.add
                local.set $4
              end ;; $block_17
              block $block_19
                local.get $4
                i32.load
                local.get $3
                i32.const 8
                i32.add
                local.get $4
                i32.const 4
                i32.add
                i32.load
                call_indirect $13 (type $2)
                i32.eqz
                br_if $block_19
                i32.const 1
                local.set $4
                br $block
              end ;; $block_19
              local.get $6
              local.get $10
              i32.ge_u
              br_if $block_2
              local.get $2
              i32.const 4
              i32.add
              local.set $0
              local.get $2
              i32.load
              local.set $1
              local.get $5
              i32.const 36
              i32.add
              local.set $5
              local.get $2
              i32.const 8
              i32.add
              local.set $2
              i32.const 1
              local.set $4
              local.get $6
              i32.const 1
              i32.add
              local.set $6
              local.get $3
              i32.load offset=32
              local.get $1
              local.get $0
              i32.load
              local.get $3
              i32.load offset=36
              i32.load offset=12
              call_indirect $13 (type $1)
              i32.eqz
              br_if $loop_0
              br $block
            end ;; $loop_0
          end ;; $block_2
          block $block_20
            local.get $9
            local.get $6
            i32.le_u
            br_if $block_20
            i32.const 1
            local.set $4
            local.get $3
            i32.load offset=32
            local.get $8
            local.get $6
            i32.const 3
            i32.shl
            i32.add
            local.tee $5
            i32.load
            local.get $5
            i32.load offset=4
            local.get $3
            i32.load offset=36
            i32.load offset=12
            call_indirect $13 (type $1)
            br_if $block
          end ;; $block_20
          i32.const 0
          local.set $4
          br $block
        end ;; $block_1
        i32.const 1049644
        call $core::panicking::panic::h540eb5ee6ff533c2
        unreachable
      end ;; $block_0
      i32.const 1049836
      local.get $4
      local.get $0
      call $core::panicking::panic_bounds_check::hb65bbcdf415380e4
      unreachable
    end ;; $block
    local.get $3
    i32.const 64
    i32.add
    global.set $15
    local.get $4
    )
  
  (func $<&T_as_core::fmt::Display>::fmt::h678bffb9cdc14530 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    global.get $15
    i32.const 32
    i32.sub
    local.tee $2
    global.set $15
    local.get $1
    i32.const 28
    i32.add
    i32.load
    local.set $3
    local.get $1
    i32.load offset=24
    local.set $4
    local.get $2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get $0
    i32.load
    local.tee $1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $2
    local.get $1
    i64.load align=4
    i64.store offset=8
    local.get $4
    local.get $3
    local.get $2
    i32.const 8
    i32.add
    call $core::fmt::write::h9ffb25a5a03fc281
    local.set $1
    local.get $2
    i32.const 32
    i32.add
    global.set $15
    local.get $1
    )
  
  (func $alloc::raw_vec::RawVec<T_2c_A>::allocate_in::_7b__7b_closure_7d__7d_::ha9193fd2beb4c622 (type $6)
    call $alloc::raw_vec::capacity_overflow::h8b890ddaa46c20bf
    unreachable
    )
  
  (func $<proxy_wasm::Logger_as_log::Log>::enabled::hd59b56d376e99849 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    i32.const 1
    )
  
  (func $<proxy_wasm::Logger_as_log::Log>::log::h69caf1c35822af17 (type $0)
    (param $0 i32)
    (param $1 i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    global.get $15
    i32.const 64
    i32.sub
    local.tee $2
    global.set $15
    local.get $1
    i32.load
    local.set $3
    local.get $2
    local.get $1
    i32.const 12
    i32.add
    i32.store offset=4
    local.get $2
    i32.const 0
    i32.store offset=16
    local.get $2
    i64.const 1
    i64.store offset=8
    local.get $2
    i32.const 4
    i32.store offset=28
    local.get $2
    local.get $2
    i32.const 4
    i32.add
    i32.store offset=24
    local.get $2
    local.get $2
    i32.const 8
    i32.add
    i32.store offset=36
    local.get $2
    i32.const 60
    i32.add
    i32.const 1
    i32.store
    local.get $2
    i64.const 1
    i64.store offset=44 align=4
    local.get $2
    i32.const 1048832
    i32.store offset=40
    local.get $2
    local.get $2
    i32.const 24
    i32.add
    i32.store offset=56
    block $block
      block $block_0
        block $block_1
          local.get $2
          i32.const 36
          i32.add
          i32.const 1048724
          local.get $2
          i32.const 40
          i32.add
          call $core::fmt::write::h9ffb25a5a03fc281
          br_if $block_1
          block $block_2
            block $block_3
              local.get $2
              i32.load offset=12
              local.tee $1
              local.get $2
              i32.load offset=16
              local.tee $4
              i32.ne
              br_if $block_3
              local.get $2
              i32.load offset=8
              local.set $5
              br $block_2
            end ;; $block_3
            local.get $1
            local.get $4
            i32.lt_u
            br_if $block_0
            block $block_4
              local.get $4
              br_if $block_4
              block $block_5
                local.get $1
                i32.eqz
                br_if $block_5
                local.get $2
                i32.load offset=8
                call $__rust_dealloc
              end ;; $block_5
              local.get $2
              i64.const 1
              i64.store offset=8
              i32.const 1
              local.set $5
              i32.const 0
              local.set $1
              br $block_2
            end ;; $block_4
            local.get $2
            i32.load offset=8
            local.get $4
            call $__rust_realloc
            local.tee $5
            i32.eqz
            br_if $block
            local.get $2
            local.get $4
            i32.store offset=12
            local.get $2
            local.get $5
            i32.store offset=8
            local.get $4
            local.set $1
          end ;; $block_2
          i32.const 5
          local.get $3
          i32.sub
          local.get $5
          local.get $2
          i32.load offset=16
          call $_proxy_log
          drop
          block $block_6
            local.get $1
            i32.eqz
            br_if $block_6
            local.get $5
            call $__rust_dealloc
          end ;; $block_6
          local.get $2
          i32.const 64
          i32.add
          global.set $15
          return
        end ;; $block_1
        i32.const 1048840
        i32.const 55
        local.get $2
        i32.const 40
        i32.add
        i32.const 1048896
        call $core::result::unwrap_failed::h00271bebc73f1849
        unreachable
      end ;; $block_0
      i32.const 1048748
      call $core::panicking::panic::h540eb5ee6ff533c2
      unreachable
    end ;; $block
    local.get $4
    i32.const 1
    i32.const 0
    i32.load offset=1055156
    local.tee $2
    i32.const 2
    local.get $2
    select
    call_indirect $13 (type $0)
    unreachable
    unreachable
    )
  
  (func $__rust_dealloc (type $5)
    (param $0 i32)
    local.get $0
    call $__rdl_dealloc
    )
  
  (func $core::result::unwrap_failed::h00271bebc73f1849 (type $7)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (local $4 i32)
    global.get $15
    i32.const 64
    i32.sub
    local.tee $4
    global.set $15
    local.get $4
    local.get $1
    i32.store offset=12
    local.get $4
    local.get $0
    i32.store offset=8
    local.get $4
    local.get $3
    i32.store offset=20
    local.get $4
    local.get $2
    i32.store offset=16
    local.get $4
    i32.const 44
    i32.add
    i32.const 2
    i32.store
    local.get $4
    i32.const 60
    i32.add
    i32.const 5
    i32.store
    local.get $4
    i64.const 2
    i64.store offset=28 align=4
    local.get $4
    i32.const 1054552
    i32.store offset=24
    local.get $4
    i32.const 1
    i32.store offset=52
    local.get $4
    local.get $4
    i32.const 48
    i32.add
    i32.store offset=40
    local.get $4
    local.get $4
    i32.const 16
    i32.add
    i32.store offset=56
    local.get $4
    local.get $4
    i32.const 8
    i32.add
    i32.store offset=48
    local.get $4
    i32.const 24
    i32.add
    i32.const 1054572
    call $core::panicking::panic_fmt::h772cfe55da576359
    unreachable
    )
  
  (func $core::panicking::panic::h540eb5ee6ff533c2 (type $5)
    (param $0 i32)
    (local $1 i32)
    (local $2 i64)
    (local $3 i64)
    (local $4 i64)
    global.get $15
    i32.const 48
    i32.sub
    local.tee $1
    global.set $15
    local.get $0
    i64.load offset=8 align=4
    local.set $2
    local.get $0
    i64.load offset=16 align=4
    local.set $3
    local.get $0
    i64.load align=4
    local.set $4
    local.get $1
    i32.const 20
    i32.add
    i32.const 0
    i32.store
    local.get $1
    i32.const 1049116
    i32.store offset=16
    local.get $1
    i64.const 1
    i64.store offset=4 align=4
    local.get $1
    local.get $4
    i64.store offset=24
    local.get $1
    local.get $1
    i32.const 24
    i32.add
    i32.store
    local.get $1
    local.get $3
    i64.store offset=40
    local.get $1
    local.get $2
    i64.store offset=32
    local.get $1
    local.get $1
    i32.const 32
    i32.add
    call $core::panicking::panic_fmt::h772cfe55da576359
    unreachable
    )
  
  (func $__post_instantiate (type $6)
    (local $0 i32)
    (local $1 i32)
    global.get $15
    i32.const 16
    i32.sub
    local.tee $0
    global.set $15
    i32.const 0
    i32.const 0
    i32.load offset=1054684
    local.tee $1
    i32.const 1
    local.get $1
    select
    i32.store offset=1054684
    block $block
      block $block_0
        local.get $1
        i32.const 1
        i32.gt_u
        br_if $block_0
        block $block_1
          local.get $1
          br_table
            $block $block_1
            $block ;; default
        end ;; $block_1
        loop $loop
          i32.const 0
          i32.load offset=1054684
          i32.const 1
          i32.eq
          br_if $loop
        end ;; $loop
      end ;; $block_0
      i32.const 1048912
      i32.const 43
      local.get $0
      i32.const 8
      i32.add
      i32.const 1048956
      call $core::result::unwrap_failed::h00271bebc73f1849
      unreachable
    end ;; $block
    i32.const 0
    i32.const 1048808
    i32.store offset=1055164
    i32.const 0
    i32.const 1049116
    i32.store offset=1055160
    i32.const 0
    i32.const 2
    i32.store offset=1054684
    i32.const 0
    i32.const 5
    i32.store offset=1054680
    local.get $0
    i32.const 16
    i32.add
    global.set $15
    )
  
  (func $_malloc (type $8)
    (param $0 i32)
    (result i32)
    (local $1 i32)
    block $block
      block $block_0
        local.get $0
        i32.const -1
        i32.le_s
        br_if $block_0
        block $block_1
          local.get $0
          br_if $block_1
          i32.const 1
          return
        end ;; $block_1
        local.get $0
        i32.const 1
        call $__rust_alloc
        local.tee $1
        i32.eqz
        br_if $block
        local.get $1
        return
      end ;; $block_0
      call $alloc::raw_vec::RawVec<T_2c_A>::allocate_in::_7b__7b_closure_7d__7d_::ha9193fd2beb4c622
      unreachable
    end ;; $block
    local.get $0
    i32.const 1
    i32.const 0
    i32.load offset=1055156
    local.tee $1
    i32.const 2
    local.get $1
    select
    call_indirect $13 (type $0)
    unreachable
    unreachable
    )
  
  (func $_free (type $5)
    (param $0 i32)
    block $block
      local.get $0
      i32.eqz
      br_if $block
      local.get $0
      call $__rust_dealloc
    end ;; $block
    )
  
  (func $<log::NopLogger_as_log::Log>::enabled::h4d0d6b60212bce89 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    i32.const 0
    )
  
  (func $<log::NopLogger_as_log::Log>::log::hab97ea506f147727 (type $0)
    (param $0 i32)
    (param $1 i32)
    )
  
  (func $<log::NopLogger_as_log::Log>::flush::hf78c792c6fe83c70 (type $5)
    (param $0 i32)
    )
  
  (func $core::ptr::real_drop_in_place::h38d0d042ad615819 (type $5)
    (param $0 i32)
    )
  
  (func $<log::SetLoggerError_as_core::fmt::Debug>::fmt::h323d5f06fc0398c5 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    global.get $15
    i32.const 80
    i32.sub
    local.tee $2
    global.set $15
    i32.const 1
    local.set $3
    block $block
      local.get $1
      i32.load offset=24
      i32.const 1049020
      i32.const 14
      local.get $1
      i32.const 28
      i32.add
      i32.load
      i32.load offset=12
      call_indirect $13 (type $1)
      br_if $block
      local.get $1
      i32.load offset=24
      local.set $4
      local.get $1
      i32.load offset=28
      i32.load offset=12
      local.set $5
      block $block_0
        block $block_1
          local.get $1
          i32.load8_u
          i32.const 4
          i32.and
          br_if $block_1
          i32.const 1
          local.set $3
          local.get $4
          i32.const 1054668
          i32.const 1
          local.get $5
          call_indirect $13 (type $1)
          br_if $block
          local.get $1
          i32.const 1049034
          i32.const 2
          call $core::fmt::Formatter::pad::h65a2184638dd238f
          i32.eqz
          br_if $block_0
          br $block
        end ;; $block_1
        local.get $4
        i32.const 1054669
        i32.const 2
        local.get $5
        call_indirect $13 (type $1)
        br_if $block
        local.get $1
        i32.load
        local.set $4
        i32.const 1
        local.set $3
        local.get $2
        i32.const 1
        i32.store8 offset=23
        local.get $2
        i32.const 52
        i32.add
        i32.const 1054612
        i32.store
        local.get $2
        local.get $4
        i32.store offset=24
        local.get $2
        local.get $1
        i64.load offset=24 align=4
        i64.store offset=8
        local.get $2
        local.get $1
        i32.load8_u offset=48
        i32.store8 offset=72
        local.get $2
        local.get $1
        i32.load offset=4
        i32.store offset=28
        local.get $2
        local.get $1
        i64.load offset=40 align=4
        i64.store offset=64
        local.get $2
        local.get $1
        i64.load offset=32 align=4
        i64.store offset=56
        local.get $2
        local.get $1
        i64.load offset=16 align=4
        i64.store offset=40
        local.get $2
        local.get $1
        i64.load offset=8 align=4
        i64.store offset=32
        local.get $2
        local.get $2
        i32.const 23
        i32.add
        i32.store offset=16
        local.get $2
        local.get $2
        i32.const 8
        i32.add
        i32.store offset=48
        local.get $2
        i32.const 24
        i32.add
        i32.const 1049034
        i32.const 2
        call $core::fmt::Formatter::pad::h65a2184638dd238f
        br_if $block
        local.get $2
        i32.const 8
        i32.add
        i32.const 1054636
        i32.const 2
        call $<core::fmt::builders::PadAdapter_as_core::fmt::Write>::write_str::h3cfa2330727f68d0
        br_if $block
      end ;; $block_0
      local.get $1
      i32.load offset=24
      i32.const 1054609
      i32.const 1
      local.get $1
      i32.load offset=28
      i32.load offset=12
      call_indirect $13 (type $1)
      local.set $3
    end ;; $block
    local.get $2
    i32.const 80
    i32.add
    global.set $15
    local.get $3
    )
  
  (func $core::fmt::Formatter::pad::h65a2184638dd238f (type $1)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
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
    local.get $0
    i32.load offset=16
    local.set $3
    block $block
      block $block_0
        block $block_1
          block $block_2
            local.get $0
            i32.load offset=8
            local.tee $4
            i32.const 1
            i32.eq
            br_if $block_2
            local.get $3
            br_if $block_1
            local.get $0
            i32.load offset=24
            local.get $1
            local.get $2
            local.get $0
            i32.const 28
            i32.add
            i32.load
            i32.load offset=12
            call_indirect $13 (type $1)
            local.set $3
            br $block
          end ;; $block_2
          local.get $3
          i32.eqz
          br_if $block_0
        end ;; $block_1
        block $block_3
          block $block_4
            local.get $2
            br_if $block_4
            i32.const 0
            local.set $2
            br $block_3
          end ;; $block_4
          local.get $1
          local.get $2
          i32.add
          local.set $5
          local.get $0
          i32.const 20
          i32.add
          i32.load
          i32.const 1
          i32.add
          local.set $6
          i32.const 0
          local.set $7
          local.get $1
          local.set $3
          local.get $1
          local.set $8
          loop $loop
            local.get $3
            i32.const 1
            i32.add
            local.set $9
            block $block_5
              block $block_6
                block $block_7
                  local.get $3
                  i32.load8_s
                  local.tee $10
                  i32.const -1
                  i32.gt_s
                  br_if $block_7
                  block $block_8
                    block $block_9
                      local.get $9
                      local.get $5
                      i32.ne
                      br_if $block_9
                      i32.const 0
                      local.set $11
                      local.get $5
                      local.set $3
                      br $block_8
                    end ;; $block_9
                    local.get $3
                    i32.load8_u offset=1
                    i32.const 63
                    i32.and
                    local.set $11
                    local.get $3
                    i32.const 2
                    i32.add
                    local.tee $9
                    local.set $3
                  end ;; $block_8
                  local.get $10
                  i32.const 31
                  i32.and
                  local.set $12
                  block $block_10
                    local.get $10
                    i32.const 255
                    i32.and
                    local.tee $10
                    i32.const 223
                    i32.gt_u
                    br_if $block_10
                    local.get $11
                    local.get $12
                    i32.const 6
                    i32.shl
                    i32.or
                    local.set $10
                    br $block_6
                  end ;; $block_10
                  block $block_11
                    block $block_12
                      local.get $3
                      local.get $5
                      i32.ne
                      br_if $block_12
                      i32.const 0
                      local.set $13
                      local.get $5
                      local.set $14
                      br $block_11
                    end ;; $block_12
                    local.get $3
                    i32.load8_u
                    i32.const 63
                    i32.and
                    local.set $13
                    local.get $3
                    i32.const 1
                    i32.add
                    local.tee $9
                    local.set $14
                  end ;; $block_11
                  local.get $13
                  local.get $11
                  i32.const 6
                  i32.shl
                  i32.or
                  local.set $11
                  block $block_13
                    local.get $10
                    i32.const 240
                    i32.ge_u
                    br_if $block_13
                    local.get $11
                    local.get $12
                    i32.const 12
                    i32.shl
                    i32.or
                    local.set $10
                    br $block_6
                  end ;; $block_13
                  block $block_14
                    block $block_15
                      local.get $14
                      local.get $5
                      i32.ne
                      br_if $block_15
                      i32.const 0
                      local.set $10
                      local.get $9
                      local.set $3
                      br $block_14
                    end ;; $block_15
                    local.get $14
                    i32.const 1
                    i32.add
                    local.set $3
                    local.get $14
                    i32.load8_u
                    i32.const 63
                    i32.and
                    local.set $10
                  end ;; $block_14
                  local.get $11
                  i32.const 6
                  i32.shl
                  local.get $12
                  i32.const 18
                  i32.shl
                  i32.const 1835008
                  i32.and
                  i32.or
                  local.get $10
                  i32.or
                  local.tee $10
                  i32.const 1114112
                  i32.ne
                  br_if $block_5
                  br $block_3
                end ;; $block_7
                local.get $10
                i32.const 255
                i32.and
                local.set $10
              end ;; $block_6
              local.get $9
              local.set $3
            end ;; $block_5
            block $block_16
              local.get $6
              i32.const -1
              i32.add
              local.tee $6
              i32.eqz
              br_if $block_16
              local.get $7
              local.get $8
              i32.sub
              local.get $3
              i32.add
              local.set $7
              local.get $3
              local.set $8
              local.get $5
              local.get $3
              i32.ne
              br_if $loop
              br $block_3
            end ;; $block_16
          end ;; $loop
          local.get $10
          i32.const 1114112
          i32.eq
          br_if $block_3
          block $block_17
            block $block_18
              local.get $7
              i32.eqz
              br_if $block_18
              local.get $7
              local.get $2
              i32.eq
              br_if $block_18
              i32.const 0
              local.set $3
              local.get $7
              local.get $2
              i32.ge_u
              br_if $block_17
              local.get $1
              local.get $7
              i32.add
              i32.load8_s
              i32.const -64
              i32.lt_s
              br_if $block_17
            end ;; $block_18
            local.get $1
            local.set $3
          end ;; $block_17
          local.get $7
          local.get $2
          local.get $3
          select
          local.set $2
          local.get $3
          local.get $1
          local.get $3
          select
          local.set $1
        end ;; $block_3
        local.get $4
        br_if $block_0
        local.get $0
        i32.load offset=24
        local.get $1
        local.get $2
        local.get $0
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect $13 (type $1)
        return
      end ;; $block_0
      i32.const 0
      local.set $9
      block $block_19
        local.get $2
        i32.eqz
        br_if $block_19
        local.get $2
        local.set $10
        local.get $1
        local.set $3
        loop $loop_0
          local.get $9
          local.get $3
          i32.load8_u
          i32.const 192
          i32.and
          i32.const 128
          i32.eq
          i32.add
          local.set $9
          local.get $3
          i32.const 1
          i32.add
          local.set $3
          local.get $10
          i32.const -1
          i32.add
          local.tee $10
          br_if $loop_0
        end ;; $loop_0
      end ;; $block_19
      block $block_20
        local.get $2
        local.get $9
        i32.sub
        local.get $0
        i32.load offset=12
        local.tee $6
        i32.lt_u
        br_if $block_20
        local.get $0
        i32.load offset=24
        local.get $1
        local.get $2
        local.get $0
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect $13 (type $1)
        return
      end ;; $block_20
      i32.const 0
      local.set $7
      i32.const 0
      local.set $9
      block $block_21
        local.get $2
        i32.eqz
        br_if $block_21
        i32.const 0
        local.set $9
        local.get $2
        local.set $10
        local.get $1
        local.set $3
        loop $loop_1
          local.get $9
          local.get $3
          i32.load8_u
          i32.const 192
          i32.and
          i32.const 128
          i32.eq
          i32.add
          local.set $9
          local.get $3
          i32.const 1
          i32.add
          local.set $3
          local.get $10
          i32.const -1
          i32.add
          local.tee $10
          br_if $loop_1
        end ;; $loop_1
      end ;; $block_21
      local.get $9
      local.get $2
      i32.sub
      local.get $6
      i32.add
      local.set $10
      block $block_22
        block $block_23
          block $block_24
            i32.const 0
            local.get $0
            i32.load8_u offset=48
            local.tee $3
            local.get $3
            i32.const 3
            i32.eq
            select
            br_table
              $block_22 $block_24 $block_23 $block_24
              $block_22 ;; default
          end ;; $block_24
          local.get $10
          local.set $7
          i32.const 0
          local.set $10
          br $block_22
        end ;; $block_23
        local.get $10
        i32.const 1
        i32.shr_u
        local.set $7
        local.get $10
        i32.const 1
        i32.add
        i32.const 1
        i32.shr_u
        local.set $10
      end ;; $block_22
      local.get $7
      i32.const 1
      i32.add
      local.set $3
      block $block_25
        loop $loop_2
          local.get $3
          i32.const -1
          i32.add
          local.tee $3
          i32.eqz
          br_if $block_25
          local.get $0
          i32.load offset=24
          local.get $0
          i32.load offset=4
          local.get $0
          i32.load offset=28
          i32.load offset=16
          call_indirect $13 (type $2)
          i32.eqz
          br_if $loop_2
        end ;; $loop_2
        i32.const 1
        return
      end ;; $block_25
      local.get $0
      i32.load offset=4
      local.set $9
      i32.const 1
      local.set $3
      local.get $0
      i32.load offset=24
      local.get $1
      local.get $2
      local.get $0
      i32.load offset=28
      i32.load offset=12
      call_indirect $13 (type $1)
      br_if $block
      local.get $10
      i32.const 1
      i32.add
      local.set $3
      local.get $0
      i32.load offset=28
      local.set $10
      local.get $0
      i32.load offset=24
      local.set $0
      loop $loop_3
        block $block_26
          local.get $3
          i32.const -1
          i32.add
          local.tee $3
          br_if $block_26
          i32.const 0
          return
        end ;; $block_26
        local.get $0
        local.get $9
        local.get $10
        i32.load offset=16
        call_indirect $13 (type $2)
        i32.eqz
        br_if $loop_3
      end ;; $loop_3
      i32.const 1
      return
    end ;; $block
    local.get $3
    )
  
  (func $<core::fmt::builders::PadAdapter_as_core::fmt::Write>::write_str::h3cfa2330727f68d0 (type $1)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    (local $7 i32)
    (local $8 i32)
    (local $9 i32)
    global.get $15
    i32.const 48
    i32.sub
    local.tee $3
    global.set $15
    block $block
      block $block_0
        local.get $2
        br_if $block_0
        i32.const 0
        local.set $4
        br $block
      end ;; $block_0
      local.get $3
      i32.const 40
      i32.add
      local.set $5
      block $block_1
        block $block_2
          block $block_3
            block $block_4
              loop $loop
                block $block_5
                  local.get $0
                  i32.load offset=8
                  i32.load8_u
                  i32.eqz
                  br_if $block_5
                  local.get $0
                  i32.load
                  i32.const 1054664
                  i32.const 4
                  local.get $0
                  i32.load offset=4
                  i32.load offset=12
                  call_indirect $13 (type $1)
                  br_if $block_1
                end ;; $block_5
                local.get $3
                i32.const 10
                i32.store offset=40
                local.get $3
                i64.const 4294967306
                i64.store offset=32
                local.get $3
                local.get $2
                i32.store offset=28
                local.get $3
                i32.const 0
                i32.store offset=24
                local.get $3
                local.get $2
                i32.store offset=20
                local.get $3
                local.get $1
                i32.store offset=16
                local.get $3
                i32.const 8
                i32.add
                i32.const 10
                local.get $1
                local.get $2
                call $core::slice::memchr::memchr::h9a5a49faa1649096
                block $block_6
                  block $block_7
                    block $block_8
                      block $block_9
                        local.get $3
                        i32.load offset=8
                        i32.const 1
                        i32.ne
                        br_if $block_9
                        local.get $3
                        i32.load offset=12
                        local.set $4
                        loop $loop_0
                          local.get $3
                          local.get $4
                          local.get $3
                          i32.load offset=24
                          i32.add
                          i32.const 1
                          i32.add
                          local.tee $4
                          i32.store offset=24
                          block $block_10
                            block $block_11
                              local.get $4
                              local.get $3
                              i32.load offset=36
                              local.tee $6
                              i32.ge_u
                              br_if $block_11
                              local.get $3
                              i32.load offset=20
                              local.set $7
                              br $block_10
                            end ;; $block_11
                            local.get $3
                            i32.load offset=20
                            local.tee $7
                            local.get $4
                            i32.lt_u
                            br_if $block_10
                            local.get $6
                            i32.const 5
                            i32.ge_u
                            br_if $block_4
                            local.get $3
                            i32.load offset=16
                            local.get $4
                            local.get $6
                            i32.sub
                            local.tee $8
                            i32.add
                            local.tee $9
                            local.get $5
                            i32.eq
                            br_if $block_7
                            local.get $9
                            local.get $5
                            local.get $6
                            call $memcmp
                            i32.eqz
                            br_if $block_7
                          end ;; $block_10
                          local.get $3
                          i32.load offset=28
                          local.tee $9
                          local.get $4
                          i32.lt_u
                          br_if $block_8
                          local.get $7
                          local.get $9
                          i32.lt_u
                          br_if $block_8
                          local.get $3
                          local.get $6
                          local.get $3
                          i32.const 16
                          i32.add
                          i32.add
                          i32.const 23
                          i32.add
                          i32.load8_u
                          local.get $3
                          i32.load offset=16
                          local.get $4
                          i32.add
                          local.get $9
                          local.get $4
                          i32.sub
                          call $core::slice::memchr::memchr::h9a5a49faa1649096
                          local.get $3
                          i32.load offset=4
                          local.set $4
                          local.get $3
                          i32.load
                          i32.const 1
                          i32.eq
                          br_if $loop_0
                        end ;; $loop_0
                      end ;; $block_9
                      local.get $3
                      local.get $3
                      i32.load offset=28
                      i32.store offset=24
                    end ;; $block_8
                    local.get $0
                    i32.load offset=8
                    i32.const 0
                    i32.store8
                    local.get $2
                    local.set $4
                    br $block_6
                  end ;; $block_7
                  local.get $0
                  i32.load offset=8
                  i32.const 1
                  i32.store8
                  local.get $8
                  i32.const 1
                  i32.add
                  local.set $4
                end ;; $block_6
                local.get $0
                i32.load offset=4
                local.set $9
                local.get $0
                i32.load
                local.set $6
                block $block_12
                  local.get $4
                  i32.eqz
                  local.get $2
                  local.get $4
                  i32.eq
                  i32.or
                  local.tee $7
                  br_if $block_12
                  local.get $2
                  local.get $4
                  i32.le_u
                  br_if $block_3
                  local.get $1
                  local.get $4
                  i32.add
                  i32.load8_s
                  i32.const -65
                  i32.le_s
                  br_if $block_3
                end ;; $block_12
                local.get $6
                local.get $1
                local.get $4
                local.get $9
                i32.load offset=12
                call_indirect $13 (type $1)
                br_if $block_1
                block $block_13
                  local.get $7
                  br_if $block_13
                  local.get $2
                  local.get $4
                  i32.le_u
                  br_if $block_2
                  local.get $1
                  local.get $4
                  i32.add
                  i32.load8_s
                  i32.const -65
                  i32.le_s
                  br_if $block_2
                end ;; $block_13
                local.get $1
                local.get $4
                i32.add
                local.set $1
                local.get $2
                local.get $4
                i32.sub
                local.tee $2
                br_if $loop
              end ;; $loop
              i32.const 0
              local.set $4
              br $block
            end ;; $block_4
            local.get $6
            i32.const 4
            call $core::slice::slice_index_len_fail::h7c242876dcc4f7b2
            unreachable
          end ;; $block_3
          local.get $1
          local.get $2
          i32.const 0
          local.get $4
          call $core::str::slice_error_fail::he0fc563abf615243
          unreachable
        end ;; $block_2
        local.get $1
        local.get $2
        local.get $4
        local.get $2
        call $core::str::slice_error_fail::he0fc563abf615243
        unreachable
      end ;; $block_1
      i32.const 1
      local.set $4
    end ;; $block
    local.get $3
    i32.const 48
    i32.add
    global.set $15
    local.get $4
    )
  
  (func $std::panicking::rust_panic_with_hook::hd01e7017ef618665 (type $5)
    (param $0 i32)
    (local $1 i32)
    (local $2 i32)
    i32.const 1
    local.set $1
    block $block
      block $block_0
        block $block_1
          i32.const 0
          i32.load offset=1055144
          i32.const 1
          i32.eq
          br_if $block_1
          i32.const 0
          i64.const 4294967297
          i64.store offset=1055144
          br $block_0
        end ;; $block_1
        i32.const 0
        i32.const 0
        i32.load offset=1055148
        i32.const 1
        i32.add
        local.tee $1
        i32.store offset=1055148
        local.get $1
        i32.const 2
        i32.gt_u
        br_if $block
      end ;; $block_0
      i32.const 0
      i32.load offset=1055152
      local.tee $2
      i32.const -1
      i32.le_s
      br_if $block
      i32.const 0
      local.get $2
      i32.store offset=1055152
      local.get $1
      i32.const 1
      i32.gt_u
      br_if $block
      call $rust_panic
      unreachable
    end ;; $block
    unreachable
    unreachable
    )
  
  (func $rust_panic (type $6)
    unreachable
    unreachable
    )
  
  (func $std::panicking::continue_panic_fmt::he42d474e4cddb3a9 (type $5)
    (param $0 i32)
    (local $1 i32)
    global.get $15
    i32.const 16
    i32.sub
    local.tee $1
    global.set $15
    local.get $0
    i32.load offset=8
    call $core::option::Option<T>::unwrap::h129a791fa84208d0
    drop
    local.get $1
    local.get $0
    i32.const 20
    i32.add
    i64.load align=4
    i64.store offset=8
    local.get $1
    local.get $0
    i64.load offset=12 align=4
    i64.store
    local.get $1
    call $std::panicking::rust_panic_with_hook::hd01e7017ef618665
    unreachable
    )
  
  (func $core::option::Option<T>::unwrap::h129a791fa84208d0 (type $8)
    (param $0 i32)
    (result i32)
    block $block
      local.get $0
      br_if $block
      i32.const 1049644
      call $core::panicking::panic::h540eb5ee6ff533c2
      unreachable
    end ;; $block
    local.get $0
    )
  
  (func $__rdl_alloc (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    block $block
      block $block_0
        local.get $1
        i32.const 9
        i32.lt_u
        br_if $block_0
        i32.const 0
        local.set $2
        i32.const -65587
        local.get $1
        i32.const 16
        local.get $1
        i32.const 16
        i32.gt_u
        select
        local.tee $1
        i32.sub
        local.get $0
        i32.le_u
        br_if $block
        local.get $1
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
        local.tee $3
        i32.add
        i32.const 12
        i32.add
        call $dlmalloc::dlmalloc::Dlmalloc::malloc::hec92e476cdfbbd60
        local.tee $0
        i32.eqz
        br_if $block
        local.get $0
        i32.const -8
        i32.add
        local.set $2
        block $block_1
          block $block_2
            local.get $1
            i32.const -1
            i32.add
            local.tee $4
            local.get $0
            i32.and
            br_if $block_2
            local.get $2
            local.set $1
            br $block_1
          end ;; $block_2
          local.get $0
          i32.const -4
          i32.add
          local.tee $5
          i32.load
          local.tee $6
          i32.const -8
          i32.and
          local.get $4
          local.get $0
          i32.add
          i32.const 0
          local.get $1
          i32.sub
          i32.and
          i32.const -8
          i32.add
          local.tee $0
          local.get $0
          local.get $1
          i32.add
          local.get $0
          local.get $2
          i32.sub
          i32.const 16
          i32.gt_u
          select
          local.tee $1
          local.get $2
          i32.sub
          local.tee $0
          i32.sub
          local.set $4
          block $block_3
            local.get $6
            i32.const 3
            i32.and
            i32.eqz
            br_if $block_3
            local.get $1
            local.get $4
            local.get $1
            i32.load offset=4
            i32.const 1
            i32.and
            i32.or
            i32.const 2
            i32.or
            i32.store offset=4
            local.get $1
            local.get $4
            i32.add
            local.tee $4
            local.get $4
            i32.load offset=4
            i32.const 1
            i32.or
            i32.store offset=4
            local.get $5
            local.get $0
            local.get $5
            i32.load
            i32.const 1
            i32.and
            i32.or
            i32.const 2
            i32.or
            i32.store
            local.get $1
            local.get $1
            i32.load offset=4
            i32.const 1
            i32.or
            i32.store offset=4
            local.get $2
            local.get $0
            call $dlmalloc::dlmalloc::Dlmalloc::dispose_chunk::h1effa892c850b7b3
            br $block_1
          end ;; $block_3
          local.get $2
          i32.load
          local.set $2
          local.get $1
          local.get $4
          i32.store offset=4
          local.get $1
          local.get $2
          local.get $0
          i32.add
          i32.store
        end ;; $block_1
        block $block_4
          local.get $1
          i32.load offset=4
          local.tee $0
          i32.const 3
          i32.and
          i32.eqz
          br_if $block_4
          local.get $0
          i32.const -8
          i32.and
          local.tee $2
          local.get $3
          i32.const 16
          i32.add
          i32.le_u
          br_if $block_4
          local.get $1
          local.get $3
          local.get $0
          i32.const 1
          i32.and
          i32.or
          i32.const 2
          i32.or
          i32.store offset=4
          local.get $1
          local.get $3
          i32.add
          local.tee $0
          local.get $2
          local.get $3
          i32.sub
          local.tee $3
          i32.const 3
          i32.or
          i32.store offset=4
          local.get $1
          local.get $2
          i32.add
          local.tee $2
          local.get $2
          i32.load offset=4
          i32.const 1
          i32.or
          i32.store offset=4
          local.get $0
          local.get $3
          call $dlmalloc::dlmalloc::Dlmalloc::dispose_chunk::h1effa892c850b7b3
        end ;; $block_4
        local.get $1
        i32.const 8
        i32.add
        return
      end ;; $block_0
      local.get $0
      call $dlmalloc::dlmalloc::Dlmalloc::malloc::hec92e476cdfbbd60
      local.set $2
    end ;; $block
    local.get $2
    )
  
  (func $dlmalloc::dlmalloc::Dlmalloc::malloc::hec92e476cdfbbd60 (type $8)
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
    (local $10 i64)
    block $block
      block $block_0
        block $block_1
          local.get $0
          i32.const 245
          i32.lt_u
          br_if $block_1
          i32.const 0
          local.set $1
          local.get $0
          i32.const -65587
          i32.ge_u
          br_if $block
          local.get $0
          i32.const 11
          i32.add
          local.tee $0
          i32.const -8
          i32.and
          local.set $2
          i32.const 0
          i32.load offset=1054692
          local.tee $3
          i32.eqz
          br_if $block_0
          i32.const 0
          local.set $4
          block $block_2
            local.get $0
            i32.const 8
            i32.shr_u
            local.tee $0
            i32.eqz
            br_if $block_2
            i32.const 31
            local.set $4
            local.get $2
            i32.const 16777215
            i32.gt_u
            br_if $block_2
            local.get $2
            i32.const 6
            local.get $0
            i32.clz
            local.tee $0
            i32.sub
            i32.const 31
            i32.and
            i32.shr_u
            i32.const 1
            i32.and
            local.get $0
            i32.const 1
            i32.shl
            i32.sub
            i32.const 62
            i32.add
            local.set $4
          end ;; $block_2
          i32.const 0
          local.get $2
          i32.sub
          local.set $1
          block $block_3
            block $block_4
              block $block_5
                local.get $4
                i32.const 2
                i32.shl
                i32.const 1054960
                i32.add
                i32.load
                local.tee $0
                i32.eqz
                br_if $block_5
                i32.const 0
                local.set $5
                local.get $2
                i32.const 0
                i32.const 25
                local.get $4
                i32.const 1
                i32.shr_u
                i32.sub
                i32.const 31
                i32.and
                local.get $4
                i32.const 31
                i32.eq
                select
                i32.shl
                local.set $6
                i32.const 0
                local.set $7
                loop $loop
                  block $block_6
                    local.get $0
                    i32.load offset=4
                    i32.const -8
                    i32.and
                    local.tee $8
                    local.get $2
                    i32.lt_u
                    br_if $block_6
                    local.get $8
                    local.get $2
                    i32.sub
                    local.tee $8
                    local.get $1
                    i32.ge_u
                    br_if $block_6
                    local.get $8
                    local.set $1
                    local.get $0
                    local.set $7
                    local.get $8
                    br_if $block_6
                    i32.const 0
                    local.set $1
                    local.get $0
                    local.set $7
                    br $block_4
                  end ;; $block_6
                  local.get $0
                  i32.const 20
                  i32.add
                  i32.load
                  local.tee $8
                  local.get $5
                  local.get $8
                  local.get $0
                  local.get $6
                  i32.const 29
                  i32.shr_u
                  i32.const 4
                  i32.and
                  i32.add
                  i32.const 16
                  i32.add
                  i32.load
                  local.tee $0
                  i32.ne
                  select
                  local.get $5
                  local.get $8
                  select
                  local.set $5
                  local.get $6
                  i32.const 1
                  i32.shl
                  local.set $6
                  local.get $0
                  br_if $loop
                end ;; $loop
                block $block_7
                  local.get $5
                  i32.eqz
                  br_if $block_7
                  local.get $5
                  local.set $0
                  br $block_4
                end ;; $block_7
                local.get $7
                br_if $block_3
              end ;; $block_5
              i32.const 0
              local.set $7
              i32.const 2
              local.get $4
              i32.const 31
              i32.and
              i32.shl
              local.tee $0
              i32.const 0
              local.get $0
              i32.sub
              i32.or
              local.get $3
              i32.and
              local.tee $0
              i32.eqz
              br_if $block_0
              local.get $0
              i32.const 0
              local.get $0
              i32.sub
              i32.and
              i32.ctz
              i32.const 2
              i32.shl
              i32.const 1054960
              i32.add
              i32.load
              local.tee $0
              i32.eqz
              br_if $block_0
            end ;; $block_4
            loop $loop_0
              local.get $0
              i32.load offset=4
              i32.const -8
              i32.and
              local.tee $5
              local.get $2
              i32.ge_u
              local.get $5
              local.get $2
              i32.sub
              local.tee $8
              local.get $1
              i32.lt_u
              i32.and
              local.set $6
              block $block_8
                local.get $0
                i32.load offset=16
                local.tee $5
                br_if $block_8
                local.get $0
                i32.const 20
                i32.add
                i32.load
                local.set $5
              end ;; $block_8
              local.get $0
              local.get $7
              local.get $6
              select
              local.set $7
              local.get $8
              local.get $1
              local.get $6
              select
              local.set $1
              local.get $5
              local.set $0
              local.get $5
              br_if $loop_0
            end ;; $loop_0
            local.get $7
            i32.eqz
            br_if $block_0
          end ;; $block_3
          block $block_9
            i32.const 0
            i32.load offset=1055088
            local.tee $0
            local.get $2
            i32.lt_u
            br_if $block_9
            local.get $1
            local.get $0
            local.get $2
            i32.sub
            i32.ge_u
            br_if $block_0
          end ;; $block_9
          local.get $7
          i32.load offset=24
          local.set $4
          block $block_10
            block $block_11
              block $block_12
                local.get $7
                i32.load offset=12
                local.tee $5
                local.get $7
                i32.ne
                br_if $block_12
                local.get $7
                i32.const 20
                i32.const 16
                local.get $7
                i32.const 20
                i32.add
                local.tee $5
                i32.load
                local.tee $6
                select
                i32.add
                i32.load
                local.tee $0
                br_if $block_11
                i32.const 0
                local.set $5
                br $block_10
              end ;; $block_12
              local.get $7
              i32.load offset=8
              local.tee $0
              local.get $5
              i32.store offset=12
              local.get $5
              local.get $0
              i32.store offset=8
              br $block_10
            end ;; $block_11
            local.get $5
            local.get $7
            i32.const 16
            i32.add
            local.get $6
            select
            local.set $6
            loop $loop_1
              local.get $6
              local.set $8
              block $block_13
                local.get $0
                local.tee $5
                i32.const 20
                i32.add
                local.tee $6
                i32.load
                local.tee $0
                br_if $block_13
                local.get $5
                i32.const 16
                i32.add
                local.set $6
                local.get $5
                i32.load offset=16
                local.set $0
              end ;; $block_13
              local.get $0
              br_if $loop_1
            end ;; $loop_1
            local.get $8
            i32.const 0
            i32.store
          end ;; $block_10
          block $block_14
            local.get $4
            i32.eqz
            br_if $block_14
            block $block_15
              block $block_16
                local.get $7
                i32.load offset=28
                i32.const 2
                i32.shl
                i32.const 1054960
                i32.add
                local.tee $0
                i32.load
                local.get $7
                i32.ne
                br_if $block_16
                local.get $0
                local.get $5
                i32.store
                local.get $5
                br_if $block_15
                i32.const 0
                i32.const 0
                i32.load offset=1054692
                i32.const -2
                local.get $7
                i32.load offset=28
                i32.rotl
                i32.and
                i32.store offset=1054692
                br $block_14
              end ;; $block_16
              local.get $4
              i32.const 16
              i32.const 20
              local.get $4
              i32.load offset=16
              local.get $7
              i32.eq
              select
              i32.add
              local.get $5
              i32.store
              local.get $5
              i32.eqz
              br_if $block_14
            end ;; $block_15
            local.get $5
            local.get $4
            i32.store offset=24
            block $block_17
              local.get $7
              i32.load offset=16
              local.tee $0
              i32.eqz
              br_if $block_17
              local.get $5
              local.get $0
              i32.store offset=16
              local.get $0
              local.get $5
              i32.store offset=24
            end ;; $block_17
            local.get $7
            i32.const 20
            i32.add
            i32.load
            local.tee $0
            i32.eqz
            br_if $block_14
            local.get $5
            i32.const 20
            i32.add
            local.get $0
            i32.store
            local.get $0
            local.get $5
            i32.store offset=24
          end ;; $block_14
          block $block_18
            block $block_19
              local.get $1
              i32.const 16
              i32.lt_u
              br_if $block_19
              local.get $7
              local.get $2
              i32.const 3
              i32.or
              i32.store offset=4
              local.get $7
              local.get $2
              i32.add
              local.tee $2
              local.get $1
              i32.const 1
              i32.or
              i32.store offset=4
              local.get $2
              local.get $1
              i32.add
              local.get $1
              i32.store
              block $block_20
                local.get $1
                i32.const 256
                i32.lt_u
                br_if $block_20
                i32.const 0
                local.set $0
                block $block_21
                  local.get $1
                  i32.const 8
                  i32.shr_u
                  local.tee $5
                  i32.eqz
                  br_if $block_21
                  i32.const 31
                  local.set $0
                  local.get $1
                  i32.const 16777215
                  i32.gt_u
                  br_if $block_21
                  local.get $1
                  i32.const 6
                  local.get $5
                  i32.clz
                  local.tee $0
                  i32.sub
                  i32.const 31
                  i32.and
                  i32.shr_u
                  i32.const 1
                  i32.and
                  local.get $0
                  i32.const 1
                  i32.shl
                  i32.sub
                  i32.const 62
                  i32.add
                  local.set $0
                end ;; $block_21
                local.get $2
                i64.const 0
                i64.store offset=16 align=4
                local.get $2
                local.get $0
                i32.store offset=28
                local.get $0
                i32.const 2
                i32.shl
                i32.const 1054960
                i32.add
                local.set $5
                block $block_22
                  block $block_23
                    block $block_24
                      block $block_25
                        block $block_26
                          i32.const 0
                          i32.load offset=1054692
                          local.tee $6
                          i32.const 1
                          local.get $0
                          i32.const 31
                          i32.and
                          i32.shl
                          local.tee $8
                          i32.and
                          i32.eqz
                          br_if $block_26
                          local.get $5
                          i32.load
                          local.tee $6
                          i32.load offset=4
                          i32.const -8
                          i32.and
                          local.get $1
                          i32.ne
                          br_if $block_25
                          local.get $6
                          local.set $0
                          br $block_24
                        end ;; $block_26
                        i32.const 0
                        local.get $6
                        local.get $8
                        i32.or
                        i32.store offset=1054692
                        local.get $5
                        local.get $2
                        i32.store
                        local.get $2
                        local.get $5
                        i32.store offset=24
                        br $block_22
                      end ;; $block_25
                      local.get $1
                      i32.const 0
                      i32.const 25
                      local.get $0
                      i32.const 1
                      i32.shr_u
                      i32.sub
                      i32.const 31
                      i32.and
                      local.get $0
                      i32.const 31
                      i32.eq
                      select
                      i32.shl
                      local.set $5
                      loop $loop_2
                        local.get $6
                        local.get $5
                        i32.const 29
                        i32.shr_u
                        i32.const 4
                        i32.and
                        i32.add
                        i32.const 16
                        i32.add
                        local.tee $8
                        i32.load
                        local.tee $0
                        i32.eqz
                        br_if $block_23
                        local.get $5
                        i32.const 1
                        i32.shl
                        local.set $5
                        local.get $0
                        local.set $6
                        local.get $0
                        i32.load offset=4
                        i32.const -8
                        i32.and
                        local.get $1
                        i32.ne
                        br_if $loop_2
                      end ;; $loop_2
                    end ;; $block_24
                    local.get $0
                    i32.load offset=8
                    local.tee $1
                    local.get $2
                    i32.store offset=12
                    local.get $0
                    local.get $2
                    i32.store offset=8
                    local.get $2
                    i32.const 0
                    i32.store offset=24
                    local.get $2
                    local.get $0
                    i32.store offset=12
                    local.get $2
                    local.get $1
                    i32.store offset=8
                    br $block_18
                  end ;; $block_23
                  local.get $8
                  local.get $2
                  i32.store
                  local.get $2
                  local.get $6
                  i32.store offset=24
                end ;; $block_22
                local.get $2
                local.get $2
                i32.store offset=12
                local.get $2
                local.get $2
                i32.store offset=8
                br $block_18
              end ;; $block_20
              local.get $1
              i32.const 3
              i32.shr_u
              local.tee $1
              i32.const 3
              i32.shl
              i32.const 1054696
              i32.add
              local.set $0
              block $block_27
                block $block_28
                  i32.const 0
                  i32.load offset=1054688
                  local.tee $5
                  i32.const 1
                  local.get $1
                  i32.const 31
                  i32.and
                  i32.shl
                  local.tee $1
                  i32.and
                  i32.eqz
                  br_if $block_28
                  local.get $0
                  i32.load offset=8
                  local.set $1
                  br $block_27
                end ;; $block_28
                i32.const 0
                local.get $5
                local.get $1
                i32.or
                i32.store offset=1054688
                local.get $0
                local.set $1
              end ;; $block_27
              local.get $0
              local.get $2
              i32.store offset=8
              local.get $1
              local.get $2
              i32.store offset=12
              local.get $2
              local.get $0
              i32.store offset=12
              local.get $2
              local.get $1
              i32.store offset=8
              br $block_18
            end ;; $block_19
            local.get $7
            local.get $1
            local.get $2
            i32.add
            local.tee $0
            i32.const 3
            i32.or
            i32.store offset=4
            local.get $7
            local.get $0
            i32.add
            local.tee $0
            local.get $0
            i32.load offset=4
            i32.const 1
            i32.or
            i32.store offset=4
          end ;; $block_18
          local.get $7
          i32.const 8
          i32.add
          return
        end ;; $block_1
        block $block_29
          block $block_30
            block $block_31
              i32.const 0
              i32.load offset=1054688
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
              local.tee $1
              i32.const 31
              i32.and
              local.tee $5
              i32.shr_u
              local.tee $0
              i32.const 3
              i32.and
              br_if $block_31
              local.get $2
              i32.const 0
              i32.load offset=1055088
              i32.le_u
              br_if $block_0
              local.get $0
              br_if $block_30
              i32.const 0
              i32.load offset=1054692
              local.tee $0
              i32.eqz
              br_if $block_0
              local.get $0
              i32.const 0
              local.get $0
              i32.sub
              i32.and
              i32.ctz
              i32.const 2
              i32.shl
              i32.const 1054960
              i32.add
              i32.load
              local.tee $5
              i32.load offset=4
              i32.const -8
              i32.and
              local.get $2
              i32.sub
              local.set $1
              local.get $5
              local.set $6
              loop $loop_3
                block $block_32
                  local.get $5
                  i32.load offset=16
                  local.tee $0
                  br_if $block_32
                  local.get $5
                  i32.const 20
                  i32.add
                  i32.load
                  local.tee $0
                  i32.eqz
                  br_if $block_29
                end ;; $block_32
                local.get $0
                i32.load offset=4
                i32.const -8
                i32.and
                local.get $2
                i32.sub
                local.tee $5
                local.get $1
                local.get $5
                local.get $1
                i32.lt_u
                local.tee $5
                select
                local.set $1
                local.get $0
                local.get $6
                local.get $5
                select
                local.set $6
                local.get $0
                local.set $5
                br $loop_3
              end ;; $loop_3
            end ;; $block_31
            local.get $0
            i32.const -1
            i32.xor
            i32.const 1
            i32.and
            local.get $1
            i32.add
            local.tee $2
            i32.const 3
            i32.shl
            local.tee $6
            i32.const 1054704
            i32.add
            i32.load
            local.tee $0
            i32.const 8
            i32.add
            local.set $1
            block $block_33
              block $block_34
                local.get $0
                i32.load offset=8
                local.tee $5
                local.get $6
                i32.const 1054696
                i32.add
                local.tee $6
                i32.eq
                br_if $block_34
                local.get $5
                local.get $6
                i32.store offset=12
                local.get $6
                local.get $5
                i32.store offset=8
                br $block_33
              end ;; $block_34
              i32.const 0
              local.get $7
              i32.const -2
              local.get $2
              i32.rotl
              i32.and
              i32.store offset=1054688
            end ;; $block_33
            local.get $0
            local.get $2
            i32.const 3
            i32.shl
            local.tee $2
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
            br $block
          end ;; $block_30
          block $block_35
            block $block_36
              local.get $0
              local.get $5
              i32.shl
              i32.const 2
              local.get $5
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
              i32.ctz
              local.tee $1
              i32.const 3
              i32.shl
              local.tee $6
              i32.const 1054704
              i32.add
              i32.load
              local.tee $0
              i32.load offset=8
              local.tee $5
              local.get $6
              i32.const 1054696
              i32.add
              local.tee $6
              i32.eq
              br_if $block_36
              local.get $5
              local.get $6
              i32.store offset=12
              local.get $6
              local.get $5
              i32.store offset=8
              br $block_35
            end ;; $block_36
            i32.const 0
            local.get $7
            i32.const -2
            local.get $1
            i32.rotl
            i32.and
            i32.store offset=1054688
          end ;; $block_35
          local.get $0
          i32.const 8
          i32.add
          local.set $5
          local.get $0
          local.get $2
          i32.const 3
          i32.or
          i32.store offset=4
          local.get $0
          local.get $2
          i32.add
          local.tee $6
          local.get $1
          i32.const 3
          i32.shl
          local.tee $1
          local.get $2
          i32.sub
          local.tee $2
          i32.const 1
          i32.or
          i32.store offset=4
          local.get $0
          local.get $1
          i32.add
          local.get $2
          i32.store
          block $block_37
            i32.const 0
            i32.load offset=1055088
            local.tee $0
            i32.eqz
            br_if $block_37
            local.get $0
            i32.const 3
            i32.shr_u
            local.tee $7
            i32.const 3
            i32.shl
            i32.const 1054696
            i32.add
            local.set $1
            i32.const 0
            i32.load offset=1055096
            local.set $0
            block $block_38
              block $block_39
                i32.const 0
                i32.load offset=1054688
                local.tee $8
                i32.const 1
                local.get $7
                i32.const 31
                i32.and
                i32.shl
                local.tee $7
                i32.and
                i32.eqz
                br_if $block_39
                local.get $1
                i32.load offset=8
                local.set $7
                br $block_38
              end ;; $block_39
              i32.const 0
              local.get $8
              local.get $7
              i32.or
              i32.store offset=1054688
              local.get $1
              local.set $7
            end ;; $block_38
            local.get $1
            local.get $0
            i32.store offset=8
            local.get $7
            local.get $0
            i32.store offset=12
            local.get $0
            local.get $1
            i32.store offset=12
            local.get $0
            local.get $7
            i32.store offset=8
          end ;; $block_37
          i32.const 0
          local.get $6
          i32.store offset=1055096
          i32.const 0
          local.get $2
          i32.store offset=1055088
          local.get $5
          return
        end ;; $block_29
        local.get $6
        i32.load offset=24
        local.set $4
        block $block_40
          block $block_41
            block $block_42
              local.get $6
              i32.load offset=12
              local.tee $5
              local.get $6
              i32.ne
              br_if $block_42
              local.get $6
              i32.const 20
              i32.const 16
              local.get $6
              i32.const 20
              i32.add
              local.tee $5
              i32.load
              local.tee $7
              select
              i32.add
              i32.load
              local.tee $0
              br_if $block_41
              i32.const 0
              local.set $5
              br $block_40
            end ;; $block_42
            local.get $6
            i32.load offset=8
            local.tee $0
            local.get $5
            i32.store offset=12
            local.get $5
            local.get $0
            i32.store offset=8
            br $block_40
          end ;; $block_41
          local.get $5
          local.get $6
          i32.const 16
          i32.add
          local.get $7
          select
          local.set $7
          loop $loop_4
            local.get $7
            local.set $8
            block $block_43
              local.get $0
              local.tee $5
              i32.const 20
              i32.add
              local.tee $7
              i32.load
              local.tee $0
              br_if $block_43
              local.get $5
              i32.const 16
              i32.add
              local.set $7
              local.get $5
              i32.load offset=16
              local.set $0
            end ;; $block_43
            local.get $0
            br_if $loop_4
          end ;; $loop_4
          local.get $8
          i32.const 0
          i32.store
        end ;; $block_40
        block $block_44
          local.get $4
          i32.eqz
          br_if $block_44
          block $block_45
            block $block_46
              local.get $6
              i32.load offset=28
              i32.const 2
              i32.shl
              i32.const 1054960
              i32.add
              local.tee $0
              i32.load
              local.get $6
              i32.ne
              br_if $block_46
              local.get $0
              local.get $5
              i32.store
              local.get $5
              br_if $block_45
              i32.const 0
              i32.const 0
              i32.load offset=1054692
              i32.const -2
              local.get $6
              i32.load offset=28
              i32.rotl
              i32.and
              i32.store offset=1054692
              br $block_44
            end ;; $block_46
            local.get $4
            i32.const 16
            i32.const 20
            local.get $4
            i32.load offset=16
            local.get $6
            i32.eq
            select
            i32.add
            local.get $5
            i32.store
            local.get $5
            i32.eqz
            br_if $block_44
          end ;; $block_45
          local.get $5
          local.get $4
          i32.store offset=24
          block $block_47
            local.get $6
            i32.load offset=16
            local.tee $0
            i32.eqz
            br_if $block_47
            local.get $5
            local.get $0
            i32.store offset=16
            local.get $0
            local.get $5
            i32.store offset=24
          end ;; $block_47
          local.get $6
          i32.const 20
          i32.add
          i32.load
          local.tee $0
          i32.eqz
          br_if $block_44
          local.get $5
          i32.const 20
          i32.add
          local.get $0
          i32.store
          local.get $0
          local.get $5
          i32.store offset=24
        end ;; $block_44
        block $block_48
          block $block_49
            local.get $1
            i32.const 16
            i32.lt_u
            br_if $block_49
            local.get $6
            local.get $2
            i32.const 3
            i32.or
            i32.store offset=4
            local.get $6
            local.get $2
            i32.add
            local.tee $2
            local.get $1
            i32.const 1
            i32.or
            i32.store offset=4
            local.get $2
            local.get $1
            i32.add
            local.get $1
            i32.store
            block $block_50
              i32.const 0
              i32.load offset=1055088
              local.tee $0
              i32.eqz
              br_if $block_50
              local.get $0
              i32.const 3
              i32.shr_u
              local.tee $7
              i32.const 3
              i32.shl
              i32.const 1054696
              i32.add
              local.set $5
              i32.const 0
              i32.load offset=1055096
              local.set $0
              block $block_51
                block $block_52
                  i32.const 0
                  i32.load offset=1054688
                  local.tee $8
                  i32.const 1
                  local.get $7
                  i32.const 31
                  i32.and
                  i32.shl
                  local.tee $7
                  i32.and
                  i32.eqz
                  br_if $block_52
                  local.get $5
                  i32.load offset=8
                  local.set $7
                  br $block_51
                end ;; $block_52
                i32.const 0
                local.get $8
                local.get $7
                i32.or
                i32.store offset=1054688
                local.get $5
                local.set $7
              end ;; $block_51
              local.get $5
              local.get $0
              i32.store offset=8
              local.get $7
              local.get $0
              i32.store offset=12
              local.get $0
              local.get $5
              i32.store offset=12
              local.get $0
              local.get $7
              i32.store offset=8
            end ;; $block_50
            i32.const 0
            local.get $2
            i32.store offset=1055096
            i32.const 0
            local.get $1
            i32.store offset=1055088
            br $block_48
          end ;; $block_49
          local.get $6
          local.get $1
          local.get $2
          i32.add
          local.tee $0
          i32.const 3
          i32.or
          i32.store offset=4
          local.get $6
          local.get $0
          i32.add
          local.tee $0
          local.get $0
          i32.load offset=4
          i32.const 1
          i32.or
          i32.store offset=4
        end ;; $block_48
        local.get $6
        i32.const 8
        i32.add
        return
      end ;; $block_0
      block $block_53
        block $block_54
          block $block_55
            block $block_56
              block $block_57
                block $block_58
                  i32.const 0
                  i32.load offset=1055088
                  local.tee $0
                  local.get $2
                  i32.ge_u
                  br_if $block_58
                  i32.const 0
                  i32.load offset=1055092
                  local.tee $0
                  local.get $2
                  i32.gt_u
                  br_if $block_55
                  i32.const 0
                  local.set $1
                  local.get $2
                  i32.const 65583
                  i32.add
                  local.tee $5
                  i32.const 16
                  i32.shr_u
                  memory.grow
                  local.tee $0
                  i32.const -1
                  i32.eq
                  br_if $block
                  local.get $0
                  i32.const 16
                  i32.shl
                  local.tee $6
                  i32.eqz
                  br_if $block
                  i32.const 0
                  i32.const 0
                  i32.load offset=1055104
                  local.get $5
                  i32.const -65536
                  i32.and
                  local.tee $8
                  i32.add
                  local.tee $0
                  i32.store offset=1055104
                  i32.const 0
                  i32.const 0
                  i32.load offset=1055108
                  local.tee $1
                  local.get $0
                  local.get $0
                  local.get $1
                  i32.lt_u
                  select
                  i32.store offset=1055108
                  i32.const 0
                  i32.load offset=1055100
                  local.tee $1
                  i32.eqz
                  br_if $block_57
                  i32.const 1055112
                  local.set $0
                  loop $loop_5
                    local.get $0
                    i32.load
                    local.tee $5
                    local.get $0
                    i32.load offset=4
                    local.tee $7
                    i32.add
                    local.get $6
                    i32.eq
                    br_if $block_56
                    local.get $0
                    i32.load offset=8
                    local.tee $0
                    br_if $loop_5
                    br $block_54
                  end ;; $loop_5
                end ;; $block_58
                i32.const 0
                i32.load offset=1055096
                local.set $1
                block $block_59
                  block $block_60
                    local.get $0
                    local.get $2
                    i32.sub
                    local.tee $5
                    i32.const 15
                    i32.gt_u
                    br_if $block_60
                    i32.const 0
                    i32.const 0
                    i32.store offset=1055096
                    i32.const 0
                    i32.const 0
                    i32.store offset=1055088
                    local.get $1
                    local.get $0
                    i32.const 3
                    i32.or
                    i32.store offset=4
                    local.get $1
                    local.get $0
                    i32.add
                    local.tee $2
                    i32.const 4
                    i32.add
                    local.set $0
                    local.get $2
                    i32.load offset=4
                    i32.const 1
                    i32.or
                    local.set $2
                    br $block_59
                  end ;; $block_60
                  i32.const 0
                  local.get $5
                  i32.store offset=1055088
                  i32.const 0
                  local.get $1
                  local.get $2
                  i32.add
                  local.tee $6
                  i32.store offset=1055096
                  local.get $6
                  local.get $5
                  i32.const 1
                  i32.or
                  i32.store offset=4
                  local.get $1
                  local.get $0
                  i32.add
                  local.get $5
                  i32.store
                  local.get $2
                  i32.const 3
                  i32.or
                  local.set $2
                  local.get $1
                  i32.const 4
                  i32.add
                  local.set $0
                end ;; $block_59
                local.get $0
                local.get $2
                i32.store
                local.get $1
                i32.const 8
                i32.add
                return
              end ;; $block_57
              block $block_61
                block $block_62
                  i32.const 0
                  i32.load offset=1055132
                  local.tee $0
                  i32.eqz
                  br_if $block_62
                  local.get $0
                  local.get $6
                  i32.le_u
                  br_if $block_61
                end ;; $block_62
                i32.const 0
                local.get $6
                i32.store offset=1055132
              end ;; $block_61
              i32.const 0
              i32.const 4095
              i32.store offset=1055136
              i32.const 0
              local.get $8
              i32.store offset=1055116
              i32.const 0
              local.get $6
              i32.store offset=1055112
              i32.const 0
              i32.const 1054696
              i32.store offset=1054708
              i32.const 0
              i32.const 1054704
              i32.store offset=1054716
              i32.const 0
              i32.const 1054696
              i32.store offset=1054704
              i32.const 0
              i32.const 1054712
              i32.store offset=1054724
              i32.const 0
              i32.const 1054704
              i32.store offset=1054712
              i32.const 0
              i32.const 1054720
              i32.store offset=1054732
              i32.const 0
              i32.const 1054712
              i32.store offset=1054720
              i32.const 0
              i32.const 1054728
              i32.store offset=1054740
              i32.const 0
              i32.const 1054720
              i32.store offset=1054728
              i32.const 0
              i32.const 1054736
              i32.store offset=1054748
              i32.const 0
              i32.const 1054728
              i32.store offset=1054736
              i32.const 0
              i32.const 1054744
              i32.store offset=1054756
              i32.const 0
              i32.const 1054736
              i32.store offset=1054744
              i32.const 0
              i32.const 1054752
              i32.store offset=1054764
              i32.const 0
              i32.const 1054744
              i32.store offset=1054752
              i32.const 0
              i32.const 0
              i32.store offset=1055124
              i32.const 0
              i32.const 1054760
              i32.store offset=1054772
              i32.const 0
              i32.const 1054752
              i32.store offset=1054760
              i32.const 0
              i32.const 1054760
              i32.store offset=1054768
              i32.const 0
              i32.const 1054768
              i32.store offset=1054780
              i32.const 0
              i32.const 1054768
              i32.store offset=1054776
              i32.const 0
              i32.const 1054776
              i32.store offset=1054788
              i32.const 0
              i32.const 1054776
              i32.store offset=1054784
              i32.const 0
              i32.const 1054784
              i32.store offset=1054796
              i32.const 0
              i32.const 1054784
              i32.store offset=1054792
              i32.const 0
              i32.const 1054792
              i32.store offset=1054804
              i32.const 0
              i32.const 1054792
              i32.store offset=1054800
              i32.const 0
              i32.const 1054800
              i32.store offset=1054812
              i32.const 0
              i32.const 1054800
              i32.store offset=1054808
              i32.const 0
              i32.const 1054808
              i32.store offset=1054820
              i32.const 0
              i32.const 1054808
              i32.store offset=1054816
              i32.const 0
              i32.const 1054816
              i32.store offset=1054828
              i32.const 0
              i32.const 1054816
              i32.store offset=1054824
              i32.const 0
              i32.const 1054824
              i32.store offset=1054836
              i32.const 0
              i32.const 1054832
              i32.store offset=1054844
              i32.const 0
              i32.const 1054824
              i32.store offset=1054832
              i32.const 0
              i32.const 1054840
              i32.store offset=1054852
              i32.const 0
              i32.const 1054832
              i32.store offset=1054840
              i32.const 0
              i32.const 1054848
              i32.store offset=1054860
              i32.const 0
              i32.const 1054840
              i32.store offset=1054848
              i32.const 0
              i32.const 1054856
              i32.store offset=1054868
              i32.const 0
              i32.const 1054848
              i32.store offset=1054856
              i32.const 0
              i32.const 1054864
              i32.store offset=1054876
              i32.const 0
              i32.const 1054856
              i32.store offset=1054864
              i32.const 0
              i32.const 1054872
              i32.store offset=1054884
              i32.const 0
              i32.const 1054864
              i32.store offset=1054872
              i32.const 0
              i32.const 1054880
              i32.store offset=1054892
              i32.const 0
              i32.const 1054872
              i32.store offset=1054880
              i32.const 0
              i32.const 1054888
              i32.store offset=1054900
              i32.const 0
              i32.const 1054880
              i32.store offset=1054888
              i32.const 0
              i32.const 1054896
              i32.store offset=1054908
              i32.const 0
              i32.const 1054888
              i32.store offset=1054896
              i32.const 0
              i32.const 1054904
              i32.store offset=1054916
              i32.const 0
              i32.const 1054896
              i32.store offset=1054904
              i32.const 0
              i32.const 1054912
              i32.store offset=1054924
              i32.const 0
              i32.const 1054904
              i32.store offset=1054912
              i32.const 0
              i32.const 1054920
              i32.store offset=1054932
              i32.const 0
              i32.const 1054912
              i32.store offset=1054920
              i32.const 0
              i32.const 1054928
              i32.store offset=1054940
              i32.const 0
              i32.const 1054920
              i32.store offset=1054928
              i32.const 0
              i32.const 1054936
              i32.store offset=1054948
              i32.const 0
              i32.const 1054928
              i32.store offset=1054936
              i32.const 0
              i32.const 1054944
              i32.store offset=1054956
              i32.const 0
              i32.const 1054936
              i32.store offset=1054944
              i32.const 0
              local.get $6
              i32.store offset=1055100
              i32.const 0
              i32.const 1054944
              i32.store offset=1054952
              i32.const 0
              local.get $8
              i32.const -40
              i32.add
              local.tee $0
              i32.store offset=1055092
              local.get $6
              local.get $0
              i32.const 1
              i32.or
              i32.store offset=4
              local.get $6
              local.get $0
              i32.add
              i32.const 40
              i32.store offset=4
              i32.const 0
              i32.const 2097152
              i32.store offset=1055128
              br $block_53
            end ;; $block_56
            local.get $0
            i32.load offset=12
            br_if $block_54
            local.get $6
            local.get $1
            i32.le_u
            br_if $block_54
            local.get $5
            local.get $1
            i32.gt_u
            br_if $block_54
            local.get $0
            local.get $7
            local.get $8
            i32.add
            i32.store offset=4
            i32.const 0
            i32.const 0
            i32.load offset=1055100
            local.tee $0
            i32.const 15
            i32.add
            i32.const -8
            i32.and
            local.tee $1
            i32.const -8
            i32.add
            i32.store offset=1055100
            i32.const 0
            local.get $0
            local.get $1
            i32.sub
            i32.const 0
            i32.load offset=1055092
            local.get $8
            i32.add
            local.tee $5
            i32.add
            i32.const 8
            i32.add
            local.tee $6
            i32.store offset=1055092
            local.get $1
            i32.const -4
            i32.add
            local.get $6
            i32.const 1
            i32.or
            i32.store
            local.get $0
            local.get $5
            i32.add
            i32.const 40
            i32.store offset=4
            i32.const 0
            i32.const 2097152
            i32.store offset=1055128
            br $block_53
          end ;; $block_55
          i32.const 0
          local.get $0
          local.get $2
          i32.sub
          local.tee $1
          i32.store offset=1055092
          i32.const 0
          i32.const 0
          i32.load offset=1055100
          local.tee $0
          local.get $2
          i32.add
          local.tee $5
          i32.store offset=1055100
          local.get $5
          local.get $1
          i32.const 1
          i32.or
          i32.store offset=4
          local.get $0
          local.get $2
          i32.const 3
          i32.or
          i32.store offset=4
          local.get $0
          i32.const 8
          i32.add
          return
        end ;; $block_54
        i32.const 0
        i32.const 0
        i32.load offset=1055132
        local.tee $0
        local.get $6
        local.get $0
        local.get $6
        i32.lt_u
        select
        i32.store offset=1055132
        local.get $6
        local.get $8
        i32.add
        local.set $5
        i32.const 1055112
        local.set $0
        block $block_63
          block $block_64
            loop $loop_6
              local.get $0
              i32.load
              local.get $5
              i32.eq
              br_if $block_64
              local.get $0
              i32.load offset=8
              local.tee $0
              br_if $loop_6
              br $block_63
            end ;; $loop_6
          end ;; $block_64
          local.get $0
          i32.load offset=12
          br_if $block_63
          local.get $0
          local.get $6
          i32.store
          local.get $0
          local.get $0
          i32.load offset=4
          local.get $8
          i32.add
          i32.store offset=4
          local.get $6
          local.get $2
          i32.const 3
          i32.or
          i32.store offset=4
          local.get $6
          local.get $2
          i32.add
          local.set $0
          local.get $5
          local.get $6
          i32.sub
          local.get $2
          i32.sub
          local.set $2
          block $block_65
            block $block_66
              block $block_67
                i32.const 0
                i32.load offset=1055100
                local.get $5
                i32.eq
                br_if $block_67
                i32.const 0
                i32.load offset=1055096
                local.get $5
                i32.eq
                br_if $block_66
                block $block_68
                  local.get $5
                  i32.load offset=4
                  local.tee $1
                  i32.const 3
                  i32.and
                  i32.const 1
                  i32.ne
                  br_if $block_68
                  block $block_69
                    block $block_70
                      local.get $1
                      i32.const -8
                      i32.and
                      local.tee $3
                      i32.const 256
                      i32.lt_u
                      br_if $block_70
                      local.get $5
                      i32.load offset=24
                      local.set $9
                      block $block_71
                        block $block_72
                          block $block_73
                            local.get $5
                            i32.load offset=12
                            local.tee $7
                            local.get $5
                            i32.ne
                            br_if $block_73
                            local.get $5
                            i32.const 20
                            i32.const 16
                            local.get $5
                            i32.load offset=20
                            local.tee $7
                            select
                            i32.add
                            i32.load
                            local.tee $1
                            br_if $block_72
                            i32.const 0
                            local.set $7
                            br $block_71
                          end ;; $block_73
                          local.get $5
                          i32.load offset=8
                          local.tee $1
                          local.get $7
                          i32.store offset=12
                          local.get $7
                          local.get $1
                          i32.store offset=8
                          br $block_71
                        end ;; $block_72
                        local.get $5
                        i32.const 20
                        i32.add
                        local.get $5
                        i32.const 16
                        i32.add
                        local.get $7
                        select
                        local.set $8
                        loop $loop_7
                          local.get $8
                          local.set $4
                          block $block_74
                            local.get $1
                            local.tee $7
                            i32.const 20
                            i32.add
                            local.tee $8
                            i32.load
                            local.tee $1
                            br_if $block_74
                            local.get $7
                            i32.const 16
                            i32.add
                            local.set $8
                            local.get $7
                            i32.load offset=16
                            local.set $1
                          end ;; $block_74
                          local.get $1
                          br_if $loop_7
                        end ;; $loop_7
                        local.get $4
                        i32.const 0
                        i32.store
                      end ;; $block_71
                      local.get $9
                      i32.eqz
                      br_if $block_69
                      block $block_75
                        block $block_76
                          local.get $5
                          i32.load offset=28
                          i32.const 2
                          i32.shl
                          i32.const 1054960
                          i32.add
                          local.tee $1
                          i32.load
                          local.get $5
                          i32.ne
                          br_if $block_76
                          local.get $1
                          local.get $7
                          i32.store
                          local.get $7
                          br_if $block_75
                          i32.const 0
                          i32.const 0
                          i32.load offset=1054692
                          i32.const -2
                          local.get $5
                          i32.load offset=28
                          i32.rotl
                          i32.and
                          i32.store offset=1054692
                          br $block_69
                        end ;; $block_76
                        local.get $9
                        i32.const 16
                        i32.const 20
                        local.get $9
                        i32.load offset=16
                        local.get $5
                        i32.eq
                        select
                        i32.add
                        local.get $7
                        i32.store
                        local.get $7
                        i32.eqz
                        br_if $block_69
                      end ;; $block_75
                      local.get $7
                      local.get $9
                      i32.store offset=24
                      block $block_77
                        local.get $5
                        i32.load offset=16
                        local.tee $1
                        i32.eqz
                        br_if $block_77
                        local.get $7
                        local.get $1
                        i32.store offset=16
                        local.get $1
                        local.get $7
                        i32.store offset=24
                      end ;; $block_77
                      local.get $5
                      i32.load offset=20
                      local.tee $1
                      i32.eqz
                      br_if $block_69
                      local.get $7
                      i32.const 20
                      i32.add
                      local.get $1
                      i32.store
                      local.get $1
                      local.get $7
                      i32.store offset=24
                      br $block_69
                    end ;; $block_70
                    block $block_78
                      local.get $5
                      i32.load offset=12
                      local.tee $7
                      local.get $5
                      i32.load offset=8
                      local.tee $8
                      i32.eq
                      br_if $block_78
                      local.get $8
                      local.get $7
                      i32.store offset=12
                      local.get $7
                      local.get $8
                      i32.store offset=8
                      br $block_69
                    end ;; $block_78
                    i32.const 0
                    i32.const 0
                    i32.load offset=1054688
                    i32.const -2
                    local.get $1
                    i32.const 3
                    i32.shr_u
                    i32.rotl
                    i32.and
                    i32.store offset=1054688
                  end ;; $block_69
                  local.get $3
                  local.get $2
                  i32.add
                  local.set $2
                  local.get $5
                  local.get $3
                  i32.add
                  local.set $5
                end ;; $block_68
                local.get $5
                local.get $5
                i32.load offset=4
                i32.const -2
                i32.and
                i32.store offset=4
                local.get $0
                local.get $2
                i32.const 1
                i32.or
                i32.store offset=4
                local.get $0
                local.get $2
                i32.add
                local.get $2
                i32.store
                block $block_79
                  local.get $2
                  i32.const 256
                  i32.lt_u
                  br_if $block_79
                  i32.const 0
                  local.set $1
                  block $block_80
                    local.get $2
                    i32.const 8
                    i32.shr_u
                    local.tee $5
                    i32.eqz
                    br_if $block_80
                    i32.const 31
                    local.set $1
                    local.get $2
                    i32.const 16777215
                    i32.gt_u
                    br_if $block_80
                    local.get $2
                    i32.const 6
                    local.get $5
                    i32.clz
                    local.tee $1
                    i32.sub
                    i32.const 31
                    i32.and
                    i32.shr_u
                    i32.const 1
                    i32.and
                    local.get $1
                    i32.const 1
                    i32.shl
                    i32.sub
                    i32.const 62
                    i32.add
                    local.set $1
                  end ;; $block_80
                  local.get $0
                  i64.const 0
                  i64.store offset=16 align=4
                  local.get $0
                  local.get $1
                  i32.store offset=28
                  local.get $1
                  i32.const 2
                  i32.shl
                  i32.const 1054960
                  i32.add
                  local.set $5
                  block $block_81
                    block $block_82
                      block $block_83
                        block $block_84
                          block $block_85
                            i32.const 0
                            i32.load offset=1054692
                            local.tee $7
                            i32.const 1
                            local.get $1
                            i32.const 31
                            i32.and
                            i32.shl
                            local.tee $8
                            i32.and
                            i32.eqz
                            br_if $block_85
                            local.get $5
                            i32.load
                            local.tee $7
                            i32.load offset=4
                            i32.const -8
                            i32.and
                            local.get $2
                            i32.ne
                            br_if $block_84
                            local.get $7
                            local.set $1
                            br $block_83
                          end ;; $block_85
                          i32.const 0
                          local.get $7
                          local.get $8
                          i32.or
                          i32.store offset=1054692
                          local.get $5
                          local.get $0
                          i32.store
                          local.get $0
                          local.get $5
                          i32.store offset=24
                          br $block_81
                        end ;; $block_84
                        local.get $2
                        i32.const 0
                        i32.const 25
                        local.get $1
                        i32.const 1
                        i32.shr_u
                        i32.sub
                        i32.const 31
                        i32.and
                        local.get $1
                        i32.const 31
                        i32.eq
                        select
                        i32.shl
                        local.set $5
                        loop $loop_8
                          local.get $7
                          local.get $5
                          i32.const 29
                          i32.shr_u
                          i32.const 4
                          i32.and
                          i32.add
                          i32.const 16
                          i32.add
                          local.tee $8
                          i32.load
                          local.tee $1
                          i32.eqz
                          br_if $block_82
                          local.get $5
                          i32.const 1
                          i32.shl
                          local.set $5
                          local.get $1
                          local.set $7
                          local.get $1
                          i32.load offset=4
                          i32.const -8
                          i32.and
                          local.get $2
                          i32.ne
                          br_if $loop_8
                        end ;; $loop_8
                      end ;; $block_83
                      local.get $1
                      i32.load offset=8
                      local.tee $2
                      local.get $0
                      i32.store offset=12
                      local.get $1
                      local.get $0
                      i32.store offset=8
                      local.get $0
                      i32.const 0
                      i32.store offset=24
                      local.get $0
                      local.get $1
                      i32.store offset=12
                      local.get $0
                      local.get $2
                      i32.store offset=8
                      br $block_65
                    end ;; $block_82
                    local.get $8
                    local.get $0
                    i32.store
                    local.get $0
                    local.get $7
                    i32.store offset=24
                  end ;; $block_81
                  local.get $0
                  local.get $0
                  i32.store offset=12
                  local.get $0
                  local.get $0
                  i32.store offset=8
                  br $block_65
                end ;; $block_79
                local.get $2
                i32.const 3
                i32.shr_u
                local.tee $1
                i32.const 3
                i32.shl
                i32.const 1054696
                i32.add
                local.set $2
                block $block_86
                  block $block_87
                    i32.const 0
                    i32.load offset=1054688
                    local.tee $5
                    i32.const 1
                    local.get $1
                    i32.const 31
                    i32.and
                    i32.shl
                    local.tee $1
                    i32.and
                    i32.eqz
                    br_if $block_87
                    local.get $2
                    i32.load offset=8
                    local.set $1
                    br $block_86
                  end ;; $block_87
                  i32.const 0
                  local.get $5
                  local.get $1
                  i32.or
                  i32.store offset=1054688
                  local.get $2
                  local.set $1
                end ;; $block_86
                local.get $2
                local.get $0
                i32.store offset=8
                local.get $1
                local.get $0
                i32.store offset=12
                local.get $0
                local.get $2
                i32.store offset=12
                local.get $0
                local.get $1
                i32.store offset=8
                br $block_65
              end ;; $block_67
              i32.const 0
              local.get $0
              i32.store offset=1055100
              i32.const 0
              i32.const 0
              i32.load offset=1055092
              local.get $2
              i32.add
              local.tee $2
              i32.store offset=1055092
              local.get $0
              local.get $2
              i32.const 1
              i32.or
              i32.store offset=4
              br $block_65
            end ;; $block_66
            i32.const 0
            local.get $0
            i32.store offset=1055096
            i32.const 0
            i32.const 0
            i32.load offset=1055088
            local.get $2
            i32.add
            local.tee $2
            i32.store offset=1055088
            local.get $0
            local.get $2
            i32.const 1
            i32.or
            i32.store offset=4
            local.get $0
            local.get $2
            i32.add
            local.get $2
            i32.store
          end ;; $block_65
          local.get $6
          i32.const 8
          i32.add
          return
        end ;; $block_63
        i32.const 1055112
        local.set $0
        block $block_88
          loop $loop_9
            block $block_89
              local.get $0
              i32.load
              local.tee $5
              local.get $1
              i32.gt_u
              br_if $block_89
              local.get $5
              local.get $0
              i32.load offset=4
              i32.add
              local.tee $5
              local.get $1
              i32.gt_u
              br_if $block_88
            end ;; $block_89
            local.get $0
            i32.load offset=8
            local.set $0
            br $loop_9
          end ;; $loop_9
        end ;; $block_88
        i32.const 0
        local.get $6
        i32.store offset=1055100
        i32.const 0
        local.get $8
        i32.const -40
        i32.add
        local.tee $0
        i32.store offset=1055092
        local.get $6
        local.get $0
        i32.const 1
        i32.or
        i32.store offset=4
        local.get $6
        local.get $0
        i32.add
        i32.const 40
        i32.store offset=4
        i32.const 0
        i32.const 2097152
        i32.store offset=1055128
        local.get $1
        local.get $5
        i32.const -32
        i32.add
        i32.const -8
        i32.and
        i32.const -8
        i32.add
        local.tee $0
        local.get $0
        local.get $1
        i32.const 16
        i32.add
        i32.lt_u
        select
        local.tee $7
        i32.const 27
        i32.store offset=4
        i32.const 0
        i64.load offset=1055112 align=4
        local.set $10
        local.get $7
        i32.const 16
        i32.add
        i32.const 0
        i64.load offset=1055120 align=4
        i64.store align=4
        local.get $7
        local.get $10
        i64.store offset=8 align=4
        i32.const 0
        local.get $8
        i32.store offset=1055116
        i32.const 0
        local.get $6
        i32.store offset=1055112
        i32.const 0
        local.get $7
        i32.const 8
        i32.add
        i32.store offset=1055120
        i32.const 0
        i32.const 0
        i32.store offset=1055124
        local.get $7
        i32.const 28
        i32.add
        local.set $0
        loop $loop_10
          local.get $0
          i32.const 7
          i32.store
          local.get $5
          local.get $0
          i32.const 4
          i32.add
          local.tee $0
          i32.gt_u
          br_if $loop_10
        end ;; $loop_10
        local.get $7
        local.get $1
        i32.eq
        br_if $block_53
        local.get $7
        local.get $7
        i32.load offset=4
        i32.const -2
        i32.and
        i32.store offset=4
        local.get $1
        local.get $7
        local.get $1
        i32.sub
        local.tee $6
        i32.const 1
        i32.or
        i32.store offset=4
        local.get $7
        local.get $6
        i32.store
        block $block_90
          local.get $6
          i32.const 256
          i32.lt_u
          br_if $block_90
          i32.const 0
          local.set $0
          block $block_91
            local.get $6
            i32.const 8
            i32.shr_u
            local.tee $5
            i32.eqz
            br_if $block_91
            i32.const 31
            local.set $0
            local.get $6
            i32.const 16777215
            i32.gt_u
            br_if $block_91
            local.get $6
            i32.const 6
            local.get $5
            i32.clz
            local.tee $0
            i32.sub
            i32.const 31
            i32.and
            i32.shr_u
            i32.const 1
            i32.and
            local.get $0
            i32.const 1
            i32.shl
            i32.sub
            i32.const 62
            i32.add
            local.set $0
          end ;; $block_91
          local.get $1
          i64.const 0
          i64.store offset=16 align=4
          local.get $1
          i32.const 28
          i32.add
          local.get $0
          i32.store
          local.get $0
          i32.const 2
          i32.shl
          i32.const 1054960
          i32.add
          local.set $5
          block $block_92
            block $block_93
              block $block_94
                block $block_95
                  block $block_96
                    i32.const 0
                    i32.load offset=1054692
                    local.tee $7
                    i32.const 1
                    local.get $0
                    i32.const 31
                    i32.and
                    i32.shl
                    local.tee $8
                    i32.and
                    i32.eqz
                    br_if $block_96
                    local.get $5
                    i32.load
                    local.tee $7
                    i32.load offset=4
                    i32.const -8
                    i32.and
                    local.get $6
                    i32.ne
                    br_if $block_95
                    local.get $7
                    local.set $0
                    br $block_94
                  end ;; $block_96
                  i32.const 0
                  local.get $7
                  local.get $8
                  i32.or
                  i32.store offset=1054692
                  local.get $5
                  local.get $1
                  i32.store
                  local.get $1
                  i32.const 24
                  i32.add
                  local.get $5
                  i32.store
                  br $block_92
                end ;; $block_95
                local.get $6
                i32.const 0
                i32.const 25
                local.get $0
                i32.const 1
                i32.shr_u
                i32.sub
                i32.const 31
                i32.and
                local.get $0
                i32.const 31
                i32.eq
                select
                i32.shl
                local.set $5
                loop $loop_11
                  local.get $7
                  local.get $5
                  i32.const 29
                  i32.shr_u
                  i32.const 4
                  i32.and
                  i32.add
                  i32.const 16
                  i32.add
                  local.tee $8
                  i32.load
                  local.tee $0
                  i32.eqz
                  br_if $block_93
                  local.get $5
                  i32.const 1
                  i32.shl
                  local.set $5
                  local.get $0
                  local.set $7
                  local.get $0
                  i32.load offset=4
                  i32.const -8
                  i32.and
                  local.get $6
                  i32.ne
                  br_if $loop_11
                end ;; $loop_11
              end ;; $block_94
              local.get $0
              i32.load offset=8
              local.tee $5
              local.get $1
              i32.store offset=12
              local.get $0
              local.get $1
              i32.store offset=8
              local.get $1
              i32.const 24
              i32.add
              i32.const 0
              i32.store
              local.get $1
              local.get $0
              i32.store offset=12
              local.get $1
              local.get $5
              i32.store offset=8
              br $block_53
            end ;; $block_93
            local.get $8
            local.get $1
            i32.store
            local.get $1
            i32.const 24
            i32.add
            local.get $7
            i32.store
          end ;; $block_92
          local.get $1
          local.get $1
          i32.store offset=12
          local.get $1
          local.get $1
          i32.store offset=8
          br $block_53
        end ;; $block_90
        local.get $6
        i32.const 3
        i32.shr_u
        local.tee $5
        i32.const 3
        i32.shl
        i32.const 1054696
        i32.add
        local.set $0
        block $block_97
          block $block_98
            i32.const 0
            i32.load offset=1054688
            local.tee $6
            i32.const 1
            local.get $5
            i32.const 31
            i32.and
            i32.shl
            local.tee $5
            i32.and
            i32.eqz
            br_if $block_98
            local.get $0
            i32.load offset=8
            local.set $5
            br $block_97
          end ;; $block_98
          i32.const 0
          local.get $6
          local.get $5
          i32.or
          i32.store offset=1054688
          local.get $0
          local.set $5
        end ;; $block_97
        local.get $0
        local.get $1
        i32.store offset=8
        local.get $5
        local.get $1
        i32.store offset=12
        local.get $1
        local.get $0
        i32.store offset=12
        local.get $1
        local.get $5
        i32.store offset=8
      end ;; $block_53
      i32.const 0
      local.set $1
      i32.const 0
      i32.load offset=1055092
      local.tee $0
      local.get $2
      i32.le_u
      br_if $block
      i32.const 0
      local.get $0
      local.get $2
      i32.sub
      local.tee $1
      i32.store offset=1055092
      i32.const 0
      i32.const 0
      i32.load offset=1055100
      local.tee $0
      local.get $2
      i32.add
      local.tee $5
      i32.store offset=1055100
      local.get $5
      local.get $1
      i32.const 1
      i32.or
      i32.store offset=4
      local.get $0
      local.get $2
      i32.const 3
      i32.or
      i32.store offset=4
      local.get $0
      i32.const 8
      i32.add
      return
    end ;; $block
    local.get $1
    )
  
  (func $dlmalloc::dlmalloc::Dlmalloc::dispose_chunk::h1effa892c850b7b3 (type $0)
    (param $0 i32)
    (param $1 i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    (local $7 i32)
    local.get $0
    local.get $1
    i32.add
    local.set $2
    block $block
      block $block_0
        block $block_1
          block $block_2
            block $block_3
              local.get $0
              i32.load offset=4
              local.tee $3
              i32.const 1
              i32.and
              br_if $block_3
              local.get $3
              i32.const 3
              i32.and
              i32.eqz
              br_if $block_2
              local.get $0
              i32.load
              local.tee $3
              local.get $1
              i32.add
              local.set $1
              block $block_4
                i32.const 0
                i32.load offset=1055096
                local.get $0
                local.get $3
                i32.sub
                local.tee $0
                i32.ne
                br_if $block_4
                local.get $2
                i32.load offset=4
                i32.const 3
                i32.and
                i32.const 3
                i32.ne
                br_if $block_3
                i32.const 0
                local.get $1
                i32.store offset=1055088
                local.get $2
                local.get $2
                i32.load offset=4
                i32.const -2
                i32.and
                i32.store offset=4
                local.get $0
                local.get $1
                i32.const 1
                i32.or
                i32.store offset=4
                local.get $2
                local.get $1
                i32.store
                return
              end ;; $block_4
              block $block_5
                local.get $3
                i32.const 256
                i32.lt_u
                br_if $block_5
                local.get $0
                i32.load offset=24
                local.set $4
                block $block_6
                  block $block_7
                    block $block_8
                      local.get $0
                      i32.load offset=12
                      local.tee $5
                      local.get $0
                      i32.ne
                      br_if $block_8
                      local.get $0
                      i32.const 20
                      i32.const 16
                      local.get $0
                      i32.load offset=20
                      local.tee $5
                      select
                      i32.add
                      i32.load
                      local.tee $3
                      br_if $block_7
                      i32.const 0
                      local.set $5
                      br $block_6
                    end ;; $block_8
                    local.get $0
                    i32.load offset=8
                    local.tee $3
                    local.get $5
                    i32.store offset=12
                    local.get $5
                    local.get $3
                    i32.store offset=8
                    br $block_6
                  end ;; $block_7
                  local.get $0
                  i32.const 20
                  i32.add
                  local.get $0
                  i32.const 16
                  i32.add
                  local.get $5
                  select
                  local.set $6
                  loop $loop
                    local.get $6
                    local.set $7
                    block $block_9
                      local.get $3
                      local.tee $5
                      i32.const 20
                      i32.add
                      local.tee $6
                      i32.load
                      local.tee $3
                      br_if $block_9
                      local.get $5
                      i32.const 16
                      i32.add
                      local.set $6
                      local.get $5
                      i32.load offset=16
                      local.set $3
                    end ;; $block_9
                    local.get $3
                    br_if $loop
                  end ;; $loop
                  local.get $7
                  i32.const 0
                  i32.store
                end ;; $block_6
                local.get $4
                i32.eqz
                br_if $block_3
                block $block_10
                  block $block_11
                    local.get $0
                    i32.load offset=28
                    i32.const 2
                    i32.shl
                    i32.const 1054960
                    i32.add
                    local.tee $3
                    i32.load
                    local.get $0
                    i32.ne
                    br_if $block_11
                    local.get $3
                    local.get $5
                    i32.store
                    local.get $5
                    br_if $block_10
                    i32.const 0
                    i32.const 0
                    i32.load offset=1054692
                    i32.const -2
                    local.get $0
                    i32.load offset=28
                    i32.rotl
                    i32.and
                    i32.store offset=1054692
                    br $block_3
                  end ;; $block_11
                  local.get $4
                  i32.const 16
                  i32.const 20
                  local.get $4
                  i32.load offset=16
                  local.get $0
                  i32.eq
                  select
                  i32.add
                  local.get $5
                  i32.store
                  local.get $5
                  i32.eqz
                  br_if $block_3
                end ;; $block_10
                local.get $5
                local.get $4
                i32.store offset=24
                block $block_12
                  local.get $0
                  i32.load offset=16
                  local.tee $3
                  i32.eqz
                  br_if $block_12
                  local.get $5
                  local.get $3
                  i32.store offset=16
                  local.get $3
                  local.get $5
                  i32.store offset=24
                end ;; $block_12
                local.get $0
                i32.load offset=20
                local.tee $3
                i32.eqz
                br_if $block_3
                local.get $5
                i32.const 20
                i32.add
                local.get $3
                i32.store
                local.get $3
                local.get $5
                i32.store offset=24
                br $block_3
              end ;; $block_5
              block $block_13
                local.get $0
                i32.load offset=12
                local.tee $5
                local.get $0
                i32.load offset=8
                local.tee $6
                i32.eq
                br_if $block_13
                local.get $6
                local.get $5
                i32.store offset=12
                local.get $5
                local.get $6
                i32.store offset=8
                br $block_3
              end ;; $block_13
              i32.const 0
              i32.const 0
              i32.load offset=1054688
              i32.const -2
              local.get $3
              i32.const 3
              i32.shr_u
              i32.rotl
              i32.and
              i32.store offset=1054688
            end ;; $block_3
            block $block_14
              block $block_15
                local.get $2
                i32.load offset=4
                local.tee $3
                i32.const 2
                i32.and
                i32.eqz
                br_if $block_15
                local.get $2
                local.get $3
                i32.const -2
                i32.and
                i32.store offset=4
                local.get $0
                local.get $1
                i32.const 1
                i32.or
                i32.store offset=4
                local.get $0
                local.get $1
                i32.add
                local.get $1
                i32.store
                br $block_14
              end ;; $block_15
              block $block_16
                block $block_17
                  i32.const 0
                  i32.load offset=1055100
                  local.get $2
                  i32.eq
                  br_if $block_17
                  i32.const 0
                  i32.load offset=1055096
                  local.get $2
                  i32.ne
                  br_if $block_16
                  i32.const 0
                  local.get $0
                  i32.store offset=1055096
                  i32.const 0
                  i32.const 0
                  i32.load offset=1055088
                  local.get $1
                  i32.add
                  local.tee $1
                  i32.store offset=1055088
                  local.get $0
                  local.get $1
                  i32.const 1
                  i32.or
                  i32.store offset=4
                  local.get $0
                  local.get $1
                  i32.add
                  local.get $1
                  i32.store
                  return
                end ;; $block_17
                i32.const 0
                local.get $0
                i32.store offset=1055100
                i32.const 0
                i32.const 0
                i32.load offset=1055092
                local.get $1
                i32.add
                local.tee $1
                i32.store offset=1055092
                local.get $0
                local.get $1
                i32.const 1
                i32.or
                i32.store offset=4
                local.get $0
                i32.const 0
                i32.load offset=1055096
                i32.ne
                br_if $block_2
                i32.const 0
                i32.const 0
                i32.store offset=1055088
                i32.const 0
                i32.const 0
                i32.store offset=1055096
                return
              end ;; $block_16
              local.get $3
              i32.const -8
              i32.and
              local.tee $5
              local.get $1
              i32.add
              local.set $1
              block $block_18
                block $block_19
                  local.get $5
                  i32.const 256
                  i32.lt_u
                  br_if $block_19
                  local.get $2
                  i32.load offset=24
                  local.set $4
                  block $block_20
                    block $block_21
                      block $block_22
                        local.get $2
                        i32.load offset=12
                        local.tee $5
                        local.get $2
                        i32.ne
                        br_if $block_22
                        local.get $2
                        i32.const 20
                        i32.const 16
                        local.get $2
                        i32.load offset=20
                        local.tee $5
                        select
                        i32.add
                        i32.load
                        local.tee $3
                        br_if $block_21
                        i32.const 0
                        local.set $5
                        br $block_20
                      end ;; $block_22
                      local.get $2
                      i32.load offset=8
                      local.tee $3
                      local.get $5
                      i32.store offset=12
                      local.get $5
                      local.get $3
                      i32.store offset=8
                      br $block_20
                    end ;; $block_21
                    local.get $2
                    i32.const 20
                    i32.add
                    local.get $2
                    i32.const 16
                    i32.add
                    local.get $5
                    select
                    local.set $6
                    loop $loop_0
                      local.get $6
                      local.set $7
                      block $block_23
                        local.get $3
                        local.tee $5
                        i32.const 20
                        i32.add
                        local.tee $6
                        i32.load
                        local.tee $3
                        br_if $block_23
                        local.get $5
                        i32.const 16
                        i32.add
                        local.set $6
                        local.get $5
                        i32.load offset=16
                        local.set $3
                      end ;; $block_23
                      local.get $3
                      br_if $loop_0
                    end ;; $loop_0
                    local.get $7
                    i32.const 0
                    i32.store
                  end ;; $block_20
                  local.get $4
                  i32.eqz
                  br_if $block_18
                  block $block_24
                    block $block_25
                      local.get $2
                      i32.load offset=28
                      i32.const 2
                      i32.shl
                      i32.const 1054960
                      i32.add
                      local.tee $3
                      i32.load
                      local.get $2
                      i32.ne
                      br_if $block_25
                      local.get $3
                      local.get $5
                      i32.store
                      local.get $5
                      br_if $block_24
                      i32.const 0
                      i32.const 0
                      i32.load offset=1054692
                      i32.const -2
                      local.get $2
                      i32.load offset=28
                      i32.rotl
                      i32.and
                      i32.store offset=1054692
                      br $block_18
                    end ;; $block_25
                    local.get $4
                    i32.const 16
                    i32.const 20
                    local.get $4
                    i32.load offset=16
                    local.get $2
                    i32.eq
                    select
                    i32.add
                    local.get $5
                    i32.store
                    local.get $5
                    i32.eqz
                    br_if $block_18
                  end ;; $block_24
                  local.get $5
                  local.get $4
                  i32.store offset=24
                  block $block_26
                    local.get $2
                    i32.load offset=16
                    local.tee $3
                    i32.eqz
                    br_if $block_26
                    local.get $5
                    local.get $3
                    i32.store offset=16
                    local.get $3
                    local.get $5
                    i32.store offset=24
                  end ;; $block_26
                  local.get $2
                  i32.load offset=20
                  local.tee $2
                  i32.eqz
                  br_if $block_18
                  local.get $5
                  i32.const 20
                  i32.add
                  local.get $2
                  i32.store
                  local.get $2
                  local.get $5
                  i32.store offset=24
                  br $block_18
                end ;; $block_19
                block $block_27
                  local.get $2
                  i32.load offset=12
                  local.tee $5
                  local.get $2
                  i32.load offset=8
                  local.tee $2
                  i32.eq
                  br_if $block_27
                  local.get $2
                  local.get $5
                  i32.store offset=12
                  local.get $5
                  local.get $2
                  i32.store offset=8
                  br $block_18
                end ;; $block_27
                i32.const 0
                i32.const 0
                i32.load offset=1054688
                i32.const -2
                local.get $3
                i32.const 3
                i32.shr_u
                i32.rotl
                i32.and
                i32.store offset=1054688
              end ;; $block_18
              local.get $0
              local.get $1
              i32.const 1
              i32.or
              i32.store offset=4
              local.get $0
              local.get $1
              i32.add
              local.get $1
              i32.store
              local.get $0
              i32.const 0
              i32.load offset=1055096
              i32.ne
              br_if $block_14
              i32.const 0
              local.get $1
              i32.store offset=1055088
              return
            end ;; $block_14
            local.get $1
            i32.const 256
            i32.lt_u
            br_if $block
            i32.const 0
            local.set $2
            block $block_28
              local.get $1
              i32.const 8
              i32.shr_u
              local.tee $3
              i32.eqz
              br_if $block_28
              i32.const 31
              local.set $2
              local.get $1
              i32.const 16777215
              i32.gt_u
              br_if $block_28
              local.get $1
              i32.const 6
              local.get $3
              i32.clz
              local.tee $2
              i32.sub
              i32.const 31
              i32.and
              i32.shr_u
              i32.const 1
              i32.and
              local.get $2
              i32.const 1
              i32.shl
              i32.sub
              i32.const 62
              i32.add
              local.set $2
            end ;; $block_28
            local.get $0
            i64.const 0
            i64.store offset=16 align=4
            local.get $0
            i32.const 28
            i32.add
            local.get $2
            i32.store
            local.get $2
            i32.const 2
            i32.shl
            i32.const 1054960
            i32.add
            local.set $3
            block $block_29
              block $block_30
                block $block_31
                  i32.const 0
                  i32.load offset=1054692
                  local.tee $5
                  i32.const 1
                  local.get $2
                  i32.const 31
                  i32.and
                  i32.shl
                  local.tee $6
                  i32.and
                  i32.eqz
                  br_if $block_31
                  local.get $3
                  i32.load
                  local.tee $5
                  i32.load offset=4
                  i32.const -8
                  i32.and
                  local.get $1
                  i32.ne
                  br_if $block_30
                  local.get $5
                  local.set $2
                  br $block_29
                end ;; $block_31
                i32.const 0
                local.get $5
                local.get $6
                i32.or
                i32.store offset=1054692
                local.get $3
                local.get $0
                i32.store
                local.get $0
                i32.const 24
                i32.add
                local.get $3
                i32.store
                br $block_0
              end ;; $block_30
              local.get $1
              i32.const 0
              i32.const 25
              local.get $2
              i32.const 1
              i32.shr_u
              i32.sub
              i32.const 31
              i32.and
              local.get $2
              i32.const 31
              i32.eq
              select
              i32.shl
              local.set $3
              loop $loop_1
                local.get $5
                local.get $3
                i32.const 29
                i32.shr_u
                i32.const 4
                i32.and
                i32.add
                i32.const 16
                i32.add
                local.tee $6
                i32.load
                local.tee $2
                i32.eqz
                br_if $block_1
                local.get $3
                i32.const 1
                i32.shl
                local.set $3
                local.get $2
                local.set $5
                local.get $2
                i32.load offset=4
                i32.const -8
                i32.and
                local.get $1
                i32.ne
                br_if $loop_1
              end ;; $loop_1
            end ;; $block_29
            local.get $2
            i32.load offset=8
            local.tee $1
            local.get $0
            i32.store offset=12
            local.get $2
            local.get $0
            i32.store offset=8
            local.get $0
            i32.const 24
            i32.add
            i32.const 0
            i32.store
            local.get $0
            local.get $2
            i32.store offset=12
            local.get $0
            local.get $1
            i32.store offset=8
          end ;; $block_2
          return
        end ;; $block_1
        local.get $6
        local.get $0
        i32.store
        local.get $0
        i32.const 24
        i32.add
        local.get $5
        i32.store
      end ;; $block_0
      local.get $0
      local.get $0
      i32.store offset=12
      local.get $0
      local.get $0
      i32.store offset=8
      return
    end ;; $block
    local.get $1
    i32.const 3
    i32.shr_u
    local.tee $2
    i32.const 3
    i32.shl
    i32.const 1054696
    i32.add
    local.set $1
    block $block_32
      block $block_33
        i32.const 0
        i32.load offset=1054688
        local.tee $3
        i32.const 1
        local.get $2
        i32.const 31
        i32.and
        i32.shl
        local.tee $2
        i32.and
        i32.eqz
        br_if $block_33
        local.get $1
        i32.load offset=8
        local.set $2
        br $block_32
      end ;; $block_33
      i32.const 0
      local.get $3
      local.get $2
      i32.or
      i32.store offset=1054688
      local.get $1
      local.set $2
    end ;; $block_32
    local.get $1
    local.get $0
    i32.store offset=8
    local.get $2
    local.get $0
    i32.store offset=12
    local.get $0
    local.get $1
    i32.store offset=12
    local.get $0
    local.get $2
    i32.store offset=8
    )
  
  (func $__rdl_dealloc (type $5)
    (param $0 i32)
    local.get $0
    call $dlmalloc::dlmalloc::Dlmalloc::free::h6ea6f2ba0e2c950d
    )
  
  (func $dlmalloc::dlmalloc::Dlmalloc::free::h6ea6f2ba0e2c950d (type $5)
    (param $0 i32)
    (local $1 i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    (local $7 i32)
    local.get $0
    i32.const -8
    i32.add
    local.tee $1
    local.get $0
    i32.const -4
    i32.add
    i32.load
    local.tee $2
    i32.const -8
    i32.and
    local.tee $0
    i32.add
    local.set $3
    block $block
      block $block_0
        block $block_1
          block $block_2
            local.get $2
            i32.const 1
            i32.and
            br_if $block_2
            local.get $2
            i32.const 3
            i32.and
            i32.eqz
            br_if $block_1
            local.get $1
            i32.load
            local.tee $2
            local.get $0
            i32.add
            local.set $0
            block $block_3
              i32.const 0
              i32.load offset=1055096
              local.get $1
              local.get $2
              i32.sub
              local.tee $1
              i32.ne
              br_if $block_3
              local.get $3
              i32.load offset=4
              i32.const 3
              i32.and
              i32.const 3
              i32.ne
              br_if $block_2
              i32.const 0
              local.get $0
              i32.store offset=1055088
              local.get $3
              local.get $3
              i32.load offset=4
              i32.const -2
              i32.and
              i32.store offset=4
              local.get $1
              local.get $0
              i32.const 1
              i32.or
              i32.store offset=4
              local.get $1
              local.get $0
              i32.add
              local.get $0
              i32.store
              return
            end ;; $block_3
            block $block_4
              local.get $2
              i32.const 256
              i32.lt_u
              br_if $block_4
              local.get $1
              i32.load offset=24
              local.set $4
              block $block_5
                block $block_6
                  block $block_7
                    local.get $1
                    i32.load offset=12
                    local.tee $5
                    local.get $1
                    i32.ne
                    br_if $block_7
                    local.get $1
                    i32.const 20
                    i32.const 16
                    local.get $1
                    i32.load offset=20
                    local.tee $5
                    select
                    i32.add
                    i32.load
                    local.tee $2
                    br_if $block_6
                    i32.const 0
                    local.set $5
                    br $block_5
                  end ;; $block_7
                  local.get $1
                  i32.load offset=8
                  local.tee $2
                  local.get $5
                  i32.store offset=12
                  local.get $5
                  local.get $2
                  i32.store offset=8
                  br $block_5
                end ;; $block_6
                local.get $1
                i32.const 20
                i32.add
                local.get $1
                i32.const 16
                i32.add
                local.get $5
                select
                local.set $6
                loop $loop
                  local.get $6
                  local.set $7
                  block $block_8
                    local.get $2
                    local.tee $5
                    i32.const 20
                    i32.add
                    local.tee $6
                    i32.load
                    local.tee $2
                    br_if $block_8
                    local.get $5
                    i32.const 16
                    i32.add
                    local.set $6
                    local.get $5
                    i32.load offset=16
                    local.set $2
                  end ;; $block_8
                  local.get $2
                  br_if $loop
                end ;; $loop
                local.get $7
                i32.const 0
                i32.store
              end ;; $block_5
              local.get $4
              i32.eqz
              br_if $block_2
              block $block_9
                block $block_10
                  local.get $1
                  i32.load offset=28
                  i32.const 2
                  i32.shl
                  i32.const 1054960
                  i32.add
                  local.tee $2
                  i32.load
                  local.get $1
                  i32.ne
                  br_if $block_10
                  local.get $2
                  local.get $5
                  i32.store
                  local.get $5
                  br_if $block_9
                  i32.const 0
                  i32.const 0
                  i32.load offset=1054692
                  i32.const -2
                  local.get $1
                  i32.load offset=28
                  i32.rotl
                  i32.and
                  i32.store offset=1054692
                  br $block_2
                end ;; $block_10
                local.get $4
                i32.const 16
                i32.const 20
                local.get $4
                i32.load offset=16
                local.get $1
                i32.eq
                select
                i32.add
                local.get $5
                i32.store
                local.get $5
                i32.eqz
                br_if $block_2
              end ;; $block_9
              local.get $5
              local.get $4
              i32.store offset=24
              block $block_11
                local.get $1
                i32.load offset=16
                local.tee $2
                i32.eqz
                br_if $block_11
                local.get $5
                local.get $2
                i32.store offset=16
                local.get $2
                local.get $5
                i32.store offset=24
              end ;; $block_11
              local.get $1
              i32.load offset=20
              local.tee $2
              i32.eqz
              br_if $block_2
              local.get $5
              i32.const 20
              i32.add
              local.get $2
              i32.store
              local.get $2
              local.get $5
              i32.store offset=24
              br $block_2
            end ;; $block_4
            block $block_12
              local.get $1
              i32.load offset=12
              local.tee $5
              local.get $1
              i32.load offset=8
              local.tee $6
              i32.eq
              br_if $block_12
              local.get $6
              local.get $5
              i32.store offset=12
              local.get $5
              local.get $6
              i32.store offset=8
              br $block_2
            end ;; $block_12
            i32.const 0
            i32.const 0
            i32.load offset=1054688
            i32.const -2
            local.get $2
            i32.const 3
            i32.shr_u
            i32.rotl
            i32.and
            i32.store offset=1054688
          end ;; $block_2
          block $block_13
            block $block_14
              local.get $3
              i32.load offset=4
              local.tee $2
              i32.const 2
              i32.and
              i32.eqz
              br_if $block_14
              local.get $3
              local.get $2
              i32.const -2
              i32.and
              i32.store offset=4
              local.get $1
              local.get $0
              i32.const 1
              i32.or
              i32.store offset=4
              local.get $1
              local.get $0
              i32.add
              local.get $0
              i32.store
              br $block_13
            end ;; $block_14
            block $block_15
              block $block_16
                i32.const 0
                i32.load offset=1055100
                local.get $3
                i32.eq
                br_if $block_16
                i32.const 0
                i32.load offset=1055096
                local.get $3
                i32.ne
                br_if $block_15
                i32.const 0
                local.get $1
                i32.store offset=1055096
                i32.const 0
                i32.const 0
                i32.load offset=1055088
                local.get $0
                i32.add
                local.tee $0
                i32.store offset=1055088
                local.get $1
                local.get $0
                i32.const 1
                i32.or
                i32.store offset=4
                local.get $1
                local.get $0
                i32.add
                local.get $0
                i32.store
                return
              end ;; $block_16
              i32.const 0
              local.get $1
              i32.store offset=1055100
              i32.const 0
              i32.const 0
              i32.load offset=1055092
              local.get $0
              i32.add
              local.tee $0
              i32.store offset=1055092
              local.get $1
              local.get $0
              i32.const 1
              i32.or
              i32.store offset=4
              block $block_17
                local.get $1
                i32.const 0
                i32.load offset=1055096
                i32.ne
                br_if $block_17
                i32.const 0
                i32.const 0
                i32.store offset=1055088
                i32.const 0
                i32.const 0
                i32.store offset=1055096
              end ;; $block_17
              i32.const 0
              i32.load offset=1055128
              local.tee $2
              local.get $0
              i32.ge_u
              br_if $block_1
              i32.const 0
              i32.load offset=1055100
              local.tee $0
              i32.eqz
              br_if $block_1
              block $block_18
                i32.const 0
                i32.load offset=1055092
                local.tee $5
                i32.const 41
                i32.lt_u
                br_if $block_18
                i32.const 1055112
                local.set $1
                loop $loop_0
                  block $block_19
                    local.get $1
                    i32.load
                    local.tee $3
                    local.get $0
                    i32.gt_u
                    br_if $block_19
                    local.get $3
                    local.get $1
                    i32.load offset=4
                    i32.add
                    local.get $0
                    i32.gt_u
                    br_if $block_18
                  end ;; $block_19
                  local.get $1
                  i32.load offset=8
                  local.tee $1
                  br_if $loop_0
                end ;; $loop_0
              end ;; $block_18
              block $block_20
                block $block_21
                  i32.const 0
                  i32.load offset=1055120
                  local.tee $0
                  br_if $block_21
                  i32.const 4095
                  local.set $1
                  br $block_20
                end ;; $block_21
                i32.const 0
                local.set $1
                loop $loop_1
                  local.get $1
                  i32.const 1
                  i32.add
                  local.set $1
                  local.get $0
                  i32.load offset=8
                  local.tee $0
                  br_if $loop_1
                end ;; $loop_1
                local.get $1
                i32.const 4095
                local.get $1
                i32.const 4095
                i32.gt_u
                select
                local.set $1
              end ;; $block_20
              i32.const 0
              local.get $1
              i32.store offset=1055136
              local.get $5
              local.get $2
              i32.le_u
              br_if $block_1
              i32.const 0
              i32.const -1
              i32.store offset=1055128
              return
            end ;; $block_15
            local.get $2
            i32.const -8
            i32.and
            local.tee $5
            local.get $0
            i32.add
            local.set $0
            block $block_22
              block $block_23
                local.get $5
                i32.const 256
                i32.lt_u
                br_if $block_23
                local.get $3
                i32.load offset=24
                local.set $4
                block $block_24
                  block $block_25
                    block $block_26
                      local.get $3
                      i32.load offset=12
                      local.tee $5
                      local.get $3
                      i32.ne
                      br_if $block_26
                      local.get $3
                      i32.const 20
                      i32.const 16
                      local.get $3
                      i32.load offset=20
                      local.tee $5
                      select
                      i32.add
                      i32.load
                      local.tee $2
                      br_if $block_25
                      i32.const 0
                      local.set $5
                      br $block_24
                    end ;; $block_26
                    local.get $3
                    i32.load offset=8
                    local.tee $2
                    local.get $5
                    i32.store offset=12
                    local.get $5
                    local.get $2
                    i32.store offset=8
                    br $block_24
                  end ;; $block_25
                  local.get $3
                  i32.const 20
                  i32.add
                  local.get $3
                  i32.const 16
                  i32.add
                  local.get $5
                  select
                  local.set $6
                  loop $loop_2
                    local.get $6
                    local.set $7
                    block $block_27
                      local.get $2
                      local.tee $5
                      i32.const 20
                      i32.add
                      local.tee $6
                      i32.load
                      local.tee $2
                      br_if $block_27
                      local.get $5
                      i32.const 16
                      i32.add
                      local.set $6
                      local.get $5
                      i32.load offset=16
                      local.set $2
                    end ;; $block_27
                    local.get $2
                    br_if $loop_2
                  end ;; $loop_2
                  local.get $7
                  i32.const 0
                  i32.store
                end ;; $block_24
                local.get $4
                i32.eqz
                br_if $block_22
                block $block_28
                  block $block_29
                    local.get $3
                    i32.load offset=28
                    i32.const 2
                    i32.shl
                    i32.const 1054960
                    i32.add
                    local.tee $2
                    i32.load
                    local.get $3
                    i32.ne
                    br_if $block_29
                    local.get $2
                    local.get $5
                    i32.store
                    local.get $5
                    br_if $block_28
                    i32.const 0
                    i32.const 0
                    i32.load offset=1054692
                    i32.const -2
                    local.get $3
                    i32.load offset=28
                    i32.rotl
                    i32.and
                    i32.store offset=1054692
                    br $block_22
                  end ;; $block_29
                  local.get $4
                  i32.const 16
                  i32.const 20
                  local.get $4
                  i32.load offset=16
                  local.get $3
                  i32.eq
                  select
                  i32.add
                  local.get $5
                  i32.store
                  local.get $5
                  i32.eqz
                  br_if $block_22
                end ;; $block_28
                local.get $5
                local.get $4
                i32.store offset=24
                block $block_30
                  local.get $3
                  i32.load offset=16
                  local.tee $2
                  i32.eqz
                  br_if $block_30
                  local.get $5
                  local.get $2
                  i32.store offset=16
                  local.get $2
                  local.get $5
                  i32.store offset=24
                end ;; $block_30
                local.get $3
                i32.load offset=20
                local.tee $3
                i32.eqz
                br_if $block_22
                local.get $5
                i32.const 20
                i32.add
                local.get $3
                i32.store
                local.get $3
                local.get $5
                i32.store offset=24
                br $block_22
              end ;; $block_23
              block $block_31
                local.get $3
                i32.load offset=12
                local.tee $5
                local.get $3
                i32.load offset=8
                local.tee $3
                i32.eq
                br_if $block_31
                local.get $3
                local.get $5
                i32.store offset=12
                local.get $5
                local.get $3
                i32.store offset=8
                br $block_22
              end ;; $block_31
              i32.const 0
              i32.const 0
              i32.load offset=1054688
              i32.const -2
              local.get $2
              i32.const 3
              i32.shr_u
              i32.rotl
              i32.and
              i32.store offset=1054688
            end ;; $block_22
            local.get $1
            local.get $0
            i32.const 1
            i32.or
            i32.store offset=4
            local.get $1
            local.get $0
            i32.add
            local.get $0
            i32.store
            local.get $1
            i32.const 0
            i32.load offset=1055096
            i32.ne
            br_if $block_13
            i32.const 0
            local.get $0
            i32.store offset=1055088
            return
          end ;; $block_13
          local.get $0
          i32.const 256
          i32.lt_u
          br_if $block_0
          i32.const 0
          local.set $3
          block $block_32
            local.get $0
            i32.const 8
            i32.shr_u
            local.tee $2
            i32.eqz
            br_if $block_32
            i32.const 31
            local.set $3
            local.get $0
            i32.const 16777215
            i32.gt_u
            br_if $block_32
            local.get $0
            i32.const 6
            local.get $2
            i32.clz
            local.tee $3
            i32.sub
            i32.const 31
            i32.and
            i32.shr_u
            i32.const 1
            i32.and
            local.get $3
            i32.const 1
            i32.shl
            i32.sub
            i32.const 62
            i32.add
            local.set $3
          end ;; $block_32
          local.get $1
          i64.const 0
          i64.store offset=16 align=4
          local.get $1
          i32.const 28
          i32.add
          local.get $3
          i32.store
          local.get $3
          i32.const 2
          i32.shl
          i32.const 1054960
          i32.add
          local.set $2
          block $block_33
            block $block_34
              block $block_35
                block $block_36
                  block $block_37
                    block $block_38
                      i32.const 0
                      i32.load offset=1054692
                      local.tee $5
                      i32.const 1
                      local.get $3
                      i32.const 31
                      i32.and
                      i32.shl
                      local.tee $6
                      i32.and
                      i32.eqz
                      br_if $block_38
                      local.get $2
                      i32.load
                      local.tee $5
                      i32.load offset=4
                      i32.const -8
                      i32.and
                      local.get $0
                      i32.ne
                      br_if $block_37
                      local.get $5
                      local.set $3
                      br $block_36
                    end ;; $block_38
                    i32.const 0
                    local.get $5
                    local.get $6
                    i32.or
                    i32.store offset=1054692
                    local.get $2
                    local.get $1
                    i32.store
                    local.get $1
                    i32.const 24
                    i32.add
                    local.get $2
                    i32.store
                    br $block_34
                  end ;; $block_37
                  local.get $0
                  i32.const 0
                  i32.const 25
                  local.get $3
                  i32.const 1
                  i32.shr_u
                  i32.sub
                  i32.const 31
                  i32.and
                  local.get $3
                  i32.const 31
                  i32.eq
                  select
                  i32.shl
                  local.set $2
                  loop $loop_3
                    local.get $5
                    local.get $2
                    i32.const 29
                    i32.shr_u
                    i32.const 4
                    i32.and
                    i32.add
                    i32.const 16
                    i32.add
                    local.tee $6
                    i32.load
                    local.tee $3
                    i32.eqz
                    br_if $block_35
                    local.get $2
                    i32.const 1
                    i32.shl
                    local.set $2
                    local.get $3
                    local.set $5
                    local.get $3
                    i32.load offset=4
                    i32.const -8
                    i32.and
                    local.get $0
                    i32.ne
                    br_if $loop_3
                  end ;; $loop_3
                end ;; $block_36
                local.get $3
                i32.load offset=8
                local.tee $0
                local.get $1
                i32.store offset=12
                local.get $3
                local.get $1
                i32.store offset=8
                local.get $1
                i32.const 24
                i32.add
                i32.const 0
                i32.store
                local.get $1
                local.get $3
                i32.store offset=12
                local.get $1
                local.get $0
                i32.store offset=8
                br $block_33
              end ;; $block_35
              local.get $6
              local.get $1
              i32.store
              local.get $1
              i32.const 24
              i32.add
              local.get $5
              i32.store
            end ;; $block_34
            local.get $1
            local.get $1
            i32.store offset=12
            local.get $1
            local.get $1
            i32.store offset=8
          end ;; $block_33
          i32.const 0
          i32.const 0
          i32.load offset=1055136
          i32.const -1
          i32.add
          local.tee $1
          i32.store offset=1055136
          local.get $1
          i32.eqz
          br_if $block
        end ;; $block_1
        return
      end ;; $block_0
      local.get $0
      i32.const 3
      i32.shr_u
      local.tee $3
      i32.const 3
      i32.shl
      i32.const 1054696
      i32.add
      local.set $0
      block $block_39
        block $block_40
          i32.const 0
          i32.load offset=1054688
          local.tee $2
          i32.const 1
          local.get $3
          i32.const 31
          i32.and
          i32.shl
          local.tee $3
          i32.and
          i32.eqz
          br_if $block_40
          local.get $0
          i32.load offset=8
          local.set $3
          br $block_39
        end ;; $block_40
        i32.const 0
        local.get $2
        local.get $3
        i32.or
        i32.store offset=1054688
        local.get $0
        local.set $3
      end ;; $block_39
      local.get $0
      local.get $1
      i32.store offset=8
      local.get $3
      local.get $1
      i32.store offset=12
      local.get $1
      local.get $0
      i32.store offset=12
      local.get $1
      local.get $3
      i32.store offset=8
      return
    end ;; $block
    block $block_41
      block $block_42
        i32.const 0
        i32.load offset=1055120
        local.tee $0
        br_if $block_42
        i32.const 4095
        local.set $1
        br $block_41
      end ;; $block_42
      i32.const 0
      local.set $1
      loop $loop_4
        local.get $1
        i32.const 1
        i32.add
        local.set $1
        local.get $0
        i32.load offset=8
        local.tee $0
        br_if $loop_4
      end ;; $loop_4
      local.get $1
      i32.const 4095
      local.get $1
      i32.const 4095
      i32.gt_u
      select
      local.set $1
    end ;; $block_41
    i32.const 0
    local.get $1
    i32.store offset=1055136
    )
  
  (func $__rdl_realloc (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
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
    i32.const 0
    local.set $2
    block $block
      local.get $1
      i32.const -65588
      i32.gt_u
      br_if $block
      i32.const 16
      local.get $1
      i32.const 11
      i32.add
      i32.const -8
      i32.and
      local.get $1
      i32.const 11
      i32.lt_u
      select
      local.set $3
      local.get $0
      i32.const -4
      i32.add
      local.tee $4
      i32.load
      local.tee $5
      i32.const -8
      i32.and
      local.set $6
      block $block_0
        block $block_1
          block $block_2
            block $block_3
              block $block_4
                block $block_5
                  block $block_6
                    local.get $5
                    i32.const 3
                    i32.and
                    i32.eqz
                    br_if $block_6
                    local.get $0
                    i32.const -8
                    i32.add
                    local.tee $7
                    local.get $6
                    i32.add
                    local.set $8
                    local.get $6
                    local.get $3
                    i32.ge_u
                    br_if $block_5
                    i32.const 0
                    i32.load offset=1055100
                    local.get $8
                    i32.eq
                    br_if $block_4
                    i32.const 0
                    i32.load offset=1055096
                    local.get $8
                    i32.eq
                    br_if $block_3
                    local.get $8
                    i32.load offset=4
                    local.tee $5
                    i32.const 2
                    i32.and
                    br_if $block_0
                    local.get $5
                    i32.const -8
                    i32.and
                    local.tee $9
                    local.get $6
                    i32.add
                    local.tee $10
                    local.get $3
                    i32.ge_u
                    br_if $block_2
                    br $block_0
                  end ;; $block_6
                  local.get $3
                  i32.const 256
                  i32.lt_u
                  br_if $block_0
                  local.get $6
                  local.get $3
                  i32.const 4
                  i32.or
                  i32.lt_u
                  br_if $block_0
                  local.get $6
                  local.get $3
                  i32.sub
                  i32.const 131073
                  i32.ge_u
                  br_if $block_0
                  br $block_1
                end ;; $block_5
                local.get $6
                local.get $3
                i32.sub
                local.tee $1
                i32.const 16
                i32.lt_u
                br_if $block_1
                local.get $4
                local.get $3
                local.get $5
                i32.const 1
                i32.and
                i32.or
                i32.const 2
                i32.or
                i32.store
                local.get $7
                local.get $3
                i32.add
                local.tee $2
                local.get $1
                i32.const 3
                i32.or
                i32.store offset=4
                local.get $8
                local.get $8
                i32.load offset=4
                i32.const 1
                i32.or
                i32.store offset=4
                local.get $2
                local.get $1
                call $dlmalloc::dlmalloc::Dlmalloc::dispose_chunk::h1effa892c850b7b3
                br $block_1
              end ;; $block_4
              i32.const 0
              i32.load offset=1055092
              local.get $6
              i32.add
              local.tee $6
              local.get $3
              i32.le_u
              br_if $block_0
              local.get $4
              local.get $3
              local.get $5
              i32.const 1
              i32.and
              i32.or
              i32.const 2
              i32.or
              i32.store
              local.get $7
              local.get $3
              i32.add
              local.tee $1
              local.get $6
              local.get $3
              i32.sub
              local.tee $2
              i32.const 1
              i32.or
              i32.store offset=4
              i32.const 0
              local.get $2
              i32.store offset=1055092
              i32.const 0
              local.get $1
              i32.store offset=1055100
              br $block_1
            end ;; $block_3
            i32.const 0
            i32.load offset=1055088
            local.get $6
            i32.add
            local.tee $6
            local.get $3
            i32.lt_u
            br_if $block_0
            block $block_7
              block $block_8
                local.get $6
                local.get $3
                i32.sub
                local.tee $1
                i32.const 15
                i32.gt_u
                br_if $block_8
                local.get $4
                local.get $5
                i32.const 1
                i32.and
                local.get $6
                i32.or
                i32.const 2
                i32.or
                i32.store
                local.get $7
                local.get $6
                i32.add
                local.tee $1
                local.get $1
                i32.load offset=4
                i32.const 1
                i32.or
                i32.store offset=4
                i32.const 0
                local.set $1
                i32.const 0
                local.set $2
                br $block_7
              end ;; $block_8
              local.get $4
              local.get $3
              local.get $5
              i32.const 1
              i32.and
              i32.or
              i32.const 2
              i32.or
              i32.store
              local.get $7
              local.get $3
              i32.add
              local.tee $2
              local.get $1
              i32.const 1
              i32.or
              i32.store offset=4
              local.get $7
              local.get $6
              i32.add
              local.tee $3
              local.get $1
              i32.store
              local.get $3
              local.get $3
              i32.load offset=4
              i32.const -2
              i32.and
              i32.store offset=4
            end ;; $block_7
            i32.const 0
            local.get $2
            i32.store offset=1055096
            i32.const 0
            local.get $1
            i32.store offset=1055088
            br $block_1
          end ;; $block_2
          local.get $10
          local.get $3
          i32.sub
          local.set $11
          block $block_9
            block $block_10
              local.get $9
              i32.const 256
              i32.lt_u
              br_if $block_10
              local.get $8
              i32.load offset=24
              local.set $9
              block $block_11
                block $block_12
                  block $block_13
                    local.get $8
                    i32.load offset=12
                    local.tee $2
                    local.get $8
                    i32.ne
                    br_if $block_13
                    local.get $8
                    i32.const 20
                    i32.const 16
                    local.get $8
                    i32.load offset=20
                    local.tee $2
                    select
                    i32.add
                    i32.load
                    local.tee $1
                    br_if $block_12
                    i32.const 0
                    local.set $2
                    br $block_11
                  end ;; $block_13
                  local.get $8
                  i32.load offset=8
                  local.tee $1
                  local.get $2
                  i32.store offset=12
                  local.get $2
                  local.get $1
                  i32.store offset=8
                  br $block_11
                end ;; $block_12
                local.get $8
                i32.const 20
                i32.add
                local.get $8
                i32.const 16
                i32.add
                local.get $2
                select
                local.set $6
                loop $loop
                  local.get $6
                  local.set $5
                  block $block_14
                    local.get $1
                    local.tee $2
                    i32.const 20
                    i32.add
                    local.tee $6
                    i32.load
                    local.tee $1
                    br_if $block_14
                    local.get $2
                    i32.const 16
                    i32.add
                    local.set $6
                    local.get $2
                    i32.load offset=16
                    local.set $1
                  end ;; $block_14
                  local.get $1
                  br_if $loop
                end ;; $loop
                local.get $5
                i32.const 0
                i32.store
              end ;; $block_11
              local.get $9
              i32.eqz
              br_if $block_9
              block $block_15
                block $block_16
                  local.get $8
                  i32.load offset=28
                  i32.const 2
                  i32.shl
                  i32.const 1054960
                  i32.add
                  local.tee $1
                  i32.load
                  local.get $8
                  i32.ne
                  br_if $block_16
                  local.get $1
                  local.get $2
                  i32.store
                  local.get $2
                  br_if $block_15
                  i32.const 0
                  i32.const 0
                  i32.load offset=1054692
                  i32.const -2
                  local.get $8
                  i32.load offset=28
                  i32.rotl
                  i32.and
                  i32.store offset=1054692
                  br $block_9
                end ;; $block_16
                local.get $9
                i32.const 16
                i32.const 20
                local.get $9
                i32.load offset=16
                local.get $8
                i32.eq
                select
                i32.add
                local.get $2
                i32.store
                local.get $2
                i32.eqz
                br_if $block_9
              end ;; $block_15
              local.get $2
              local.get $9
              i32.store offset=24
              block $block_17
                local.get $8
                i32.load offset=16
                local.tee $1
                i32.eqz
                br_if $block_17
                local.get $2
                local.get $1
                i32.store offset=16
                local.get $1
                local.get $2
                i32.store offset=24
              end ;; $block_17
              local.get $8
              i32.load offset=20
              local.tee $1
              i32.eqz
              br_if $block_9
              local.get $2
              i32.const 20
              i32.add
              local.get $1
              i32.store
              local.get $1
              local.get $2
              i32.store offset=24
              br $block_9
            end ;; $block_10
            block $block_18
              local.get $8
              i32.load offset=12
              local.tee $1
              local.get $8
              i32.load offset=8
              local.tee $2
              i32.eq
              br_if $block_18
              local.get $2
              local.get $1
              i32.store offset=12
              local.get $1
              local.get $2
              i32.store offset=8
              br $block_9
            end ;; $block_18
            i32.const 0
            i32.const 0
            i32.load offset=1054688
            i32.const -2
            local.get $5
            i32.const 3
            i32.shr_u
            i32.rotl
            i32.and
            i32.store offset=1054688
          end ;; $block_9
          block $block_19
            local.get $11
            i32.const 16
            i32.lt_u
            br_if $block_19
            local.get $4
            local.get $3
            local.get $4
            i32.load
            i32.const 1
            i32.and
            i32.or
            i32.const 2
            i32.or
            i32.store
            local.get $7
            local.get $3
            i32.add
            local.tee $1
            local.get $11
            i32.const 3
            i32.or
            i32.store offset=4
            local.get $7
            local.get $10
            i32.add
            local.tee $2
            local.get $2
            i32.load offset=4
            i32.const 1
            i32.or
            i32.store offset=4
            local.get $1
            local.get $11
            call $dlmalloc::dlmalloc::Dlmalloc::dispose_chunk::h1effa892c850b7b3
            br $block_1
          end ;; $block_19
          local.get $4
          local.get $10
          local.get $4
          i32.load
          i32.const 1
          i32.and
          i32.or
          i32.const 2
          i32.or
          i32.store
          local.get $7
          local.get $10
          i32.add
          local.tee $1
          local.get $1
          i32.load offset=4
          i32.const 1
          i32.or
          i32.store offset=4
        end ;; $block_1
        local.get $0
        local.set $2
        br $block
      end ;; $block_0
      local.get $1
      call $dlmalloc::dlmalloc::Dlmalloc::malloc::hec92e476cdfbbd60
      local.tee $3
      i32.eqz
      br_if $block
      local.get $3
      local.get $0
      local.get $1
      local.get $4
      i32.load
      local.tee $2
      i32.const -8
      i32.and
      i32.const 4
      i32.const 8
      local.get $2
      i32.const 3
      i32.and
      select
      i32.sub
      local.tee $2
      local.get $2
      local.get $1
      i32.gt_u
      select
      call $memcpy
      local.set $1
      local.get $0
      call $dlmalloc::dlmalloc::Dlmalloc::free::h6ea6f2ba0e2c950d
      local.get $1
      return
    end ;; $block
    local.get $2
    )
  
  (func $rust_begin_unwind (type $5)
    (param $0 i32)
    local.get $0
    call $std::panicking::continue_panic_fmt::he42d474e4cddb3a9
    unreachable
    )
  
  (func $core::slice::slice_index_len_fail::h7c242876dcc4f7b2 (type $0)
    (param $0 i32)
    (param $1 i32)
    (local $2 i32)
    global.get $15
    i32.const 48
    i32.sub
    local.tee $2
    global.set $15
    local.get $2
    local.get $1
    i32.store offset=4
    local.get $2
    local.get $0
    i32.store
    local.get $2
    i32.const 28
    i32.add
    i32.const 2
    i32.store
    local.get $2
    i32.const 44
    i32.add
    i32.const 6
    i32.store
    local.get $2
    i64.const 2
    i64.store offset=12 align=4
    local.get $2
    i32.const 1049384
    i32.store offset=8
    local.get $2
    i32.const 6
    i32.store offset=36
    local.get $2
    local.get $2
    i32.const 32
    i32.add
    i32.store offset=24
    local.get $2
    local.get $2
    i32.const 4
    i32.add
    i32.store offset=40
    local.get $2
    local.get $2
    i32.store offset=32
    local.get $2
    i32.const 8
    i32.add
    i32.const 1049400
    call $core::panicking::panic_fmt::h772cfe55da576359
    unreachable
    )
  
  (func $core::fmt::num::imp::<impl_core::fmt::Display_for_u32>::fmt::h0a73e93540d1767b (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    local.get $0
    i64.load32_u
    local.get $1
    call $core::fmt::num::imp::fmt_u64::hbf92ea5b56adc693
    )
  
  (func $core::panicking::panic_fmt::h772cfe55da576359 (type $0)
    (param $0 i32)
    (param $1 i32)
    (local $2 i32)
    (local $3 i64)
    global.get $15
    i32.const 32
    i32.sub
    local.tee $2
    global.set $15
    local.get $1
    i64.load align=4
    local.set $3
    local.get $2
    i32.const 20
    i32.add
    local.get $1
    i64.load offset=8 align=4
    i64.store align=4
    local.get $2
    local.get $3
    i64.store offset=12 align=4
    local.get $2
    local.get $0
    i32.store offset=8
    local.get $2
    i32.const 1049116
    i32.store offset=4
    local.get $2
    i32.const 1049116
    i32.store
    local.get $2
    call $rust_begin_unwind
    unreachable
    )
  
  (func $core::panicking::panic_bounds_check::hb65bbcdf415380e4 (type $3)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (local $3 i32)
    global.get $15
    i32.const 48
    i32.sub
    local.tee $3
    global.set $15
    local.get $3
    local.get $2
    i32.store offset=4
    local.get $3
    local.get $1
    i32.store
    local.get $3
    i32.const 28
    i32.add
    i32.const 2
    i32.store
    local.get $3
    i32.const 44
    i32.add
    i32.const 6
    i32.store
    local.get $3
    i64.const 2
    i64.store offset=12 align=4
    local.get $3
    i32.const 1049100
    i32.store offset=8
    local.get $3
    i32.const 6
    i32.store offset=36
    local.get $3
    local.get $3
    i32.const 32
    i32.add
    i32.store offset=24
    local.get $3
    local.get $3
    i32.store offset=40
    local.get $3
    local.get $3
    i32.const 4
    i32.add
    i32.store offset=32
    local.get $3
    i32.const 8
    i32.add
    local.get $0
    call $core::panicking::panic_fmt::h772cfe55da576359
    unreachable
    )
  
  (func $core::fmt::num::imp::fmt_u64::hbf92ea5b56adc693 (type $9)
    (param $0 i64)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i64)
    (local $5 i32)
    (local $6 i32)
    (local $7 i32)
    global.get $15
    i32.const 48
    i32.sub
    local.tee $2
    global.set $15
    i32.const 39
    local.set $3
    block $block
      block $block_0
        local.get $0
        i64.const 10000
        i64.ge_u
        br_if $block_0
        local.get $0
        local.set $4
        br $block
      end ;; $block_0
      i32.const 39
      local.set $3
      loop $loop
        local.get $2
        i32.const 9
        i32.add
        local.get $3
        i32.add
        local.tee $5
        i32.const -4
        i32.add
        local.get $0
        local.get $0
        i64.const 10000
        i64.div_u
        local.tee $4
        i64.const -10000
        i64.mul
        i64.add
        i32.wrap_i64
        local.tee $6
        i32.const 65535
        i32.and
        i32.const 100
        i32.div_u
        local.tee $7
        i32.const 1
        i32.shl
        i32.const 1049182
        i32.add
        i32.load16_u align=1
        i32.store16 align=1
        local.get $5
        i32.const -2
        i32.add
        local.get $7
        i32.const -100
        i32.mul
        local.get $6
        i32.add
        i32.const 65535
        i32.and
        i32.const 1
        i32.shl
        i32.const 1049182
        i32.add
        i32.load16_u align=1
        i32.store16 align=1
        local.get $3
        i32.const -4
        i32.add
        local.set $3
        local.get $0
        i64.const 99999999
        i64.gt_u
        local.set $5
        local.get $4
        local.set $0
        local.get $5
        br_if $loop
      end ;; $loop
    end ;; $block
    block $block_1
      local.get $4
      i32.wrap_i64
      local.tee $5
      i32.const 99
      i32.le_s
      br_if $block_1
      local.get $2
      i32.const 9
      i32.add
      local.get $3
      i32.const -2
      i32.add
      local.tee $3
      i32.add
      local.get $4
      i32.wrap_i64
      local.tee $6
      i32.const 65535
      i32.and
      i32.const 100
      i32.div_u
      local.tee $5
      i32.const -100
      i32.mul
      local.get $6
      i32.add
      i32.const 65535
      i32.and
      i32.const 1
      i32.shl
      i32.const 1049182
      i32.add
      i32.load16_u align=1
      i32.store16 align=1
    end ;; $block_1
    block $block_2
      block $block_3
        local.get $5
        i32.const 10
        i32.lt_s
        br_if $block_3
        local.get $2
        i32.const 9
        i32.add
        local.get $3
        i32.const -2
        i32.add
        local.tee $3
        i32.add
        local.get $5
        i32.const 1
        i32.shl
        i32.const 1049182
        i32.add
        i32.load16_u align=1
        i32.store16 align=1
        br $block_2
      end ;; $block_3
      local.get $2
      i32.const 9
      i32.add
      local.get $3
      i32.const -1
      i32.add
      local.tee $3
      i32.add
      local.get $5
      i32.const 48
      i32.add
      i32.store8
    end ;; $block_2
    local.get $1
    i32.const 1049116
    i32.const 0
    local.get $2
    i32.const 9
    i32.add
    local.get $3
    i32.add
    i32.const 39
    local.get $3
    i32.sub
    call $core::fmt::Formatter::pad_integral::h0e630a1dafcc54e1
    local.set $3
    local.get $2
    i32.const 48
    i32.add
    global.set $15
    local.get $3
    )
  
  (func $<T_as_core::any::Any>::type_id::h0651535bade0637a (type $10)
    (param $0 i32)
    (result i64)
    i64.const 455982915832198545
    )
  
  (func $core::fmt::Formatter::pad_integral::h0e630a1dafcc54e1 (type $11)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (param $4 i32)
    (result i32)
    (local $5 i32)
    (local $6 i32)
    (local $7 i32)
    (local $8 i32)
    (local $9 i32)
    (local $10 i32)
    local.get $0
    i32.load
    local.tee $5
    i32.const 1
    i32.and
    local.tee $6
    local.get $4
    i32.add
    local.set $7
    block $block
      block $block_0
        local.get $5
        i32.const 4
        i32.and
        br_if $block_0
        i32.const 0
        local.set $1
        br $block
      end ;; $block_0
      i32.const 0
      local.set $8
      block $block_1
        local.get $2
        i32.eqz
        br_if $block_1
        local.get $2
        local.set $9
        local.get $1
        local.set $10
        loop $loop
          local.get $8
          local.get $10
          i32.load8_u
          i32.const 192
          i32.and
          i32.const 128
          i32.eq
          i32.add
          local.set $8
          local.get $10
          i32.const 1
          i32.add
          local.set $10
          local.get $9
          i32.const -1
          i32.add
          local.tee $9
          br_if $loop
        end ;; $loop
      end ;; $block_1
      local.get $7
      local.get $2
      i32.add
      local.get $8
      i32.sub
      local.set $7
    end ;; $block
    i32.const 43
    i32.const 1114112
    local.get $6
    select
    local.set $8
    block $block_2
      block $block_3
        local.get $0
        i32.load offset=8
        i32.const 1
        i32.eq
        br_if $block_3
        i32.const 1
        local.set $10
        local.get $0
        local.get $8
        local.get $1
        local.get $2
        call $core::fmt::Formatter::pad_integral::write_prefix::h9e67c72a92bd1433
        br_if $block_2
        local.get $0
        i32.load offset=24
        local.get $3
        local.get $4
        local.get $0
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect $13 (type $1)
        return
      end ;; $block_3
      block $block_4
        local.get $0
        i32.const 12
        i32.add
        i32.load
        local.tee $9
        local.get $7
        i32.gt_u
        br_if $block_4
        i32.const 1
        local.set $10
        local.get $0
        local.get $8
        local.get $1
        local.get $2
        call $core::fmt::Formatter::pad_integral::write_prefix::h9e67c72a92bd1433
        br_if $block_2
        local.get $0
        i32.load offset=24
        local.get $3
        local.get $4
        local.get $0
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect $13 (type $1)
        return
      end ;; $block_4
      block $block_5
        block $block_6
          local.get $5
          i32.const 8
          i32.and
          br_if $block_6
          local.get $9
          local.get $7
          i32.sub
          local.set $9
          i32.const 0
          local.set $10
          block $block_7
            block $block_8
              block $block_9
                i32.const 1
                local.get $0
                i32.load8_u offset=48
                local.tee $7
                local.get $7
                i32.const 3
                i32.eq
                select
                br_table
                  $block_7 $block_9 $block_8 $block_9
                  $block_7 ;; default
              end ;; $block_9
              local.get $9
              local.set $10
              i32.const 0
              local.set $9
              br $block_7
            end ;; $block_8
            local.get $9
            i32.const 1
            i32.shr_u
            local.set $10
            local.get $9
            i32.const 1
            i32.add
            i32.const 1
            i32.shr_u
            local.set $9
          end ;; $block_7
          local.get $10
          i32.const 1
          i32.add
          local.set $10
          loop $loop_0
            local.get $10
            i32.const -1
            i32.add
            local.tee $10
            i32.eqz
            br_if $block_5
            local.get $0
            i32.load offset=24
            local.get $0
            i32.load offset=4
            local.get $0
            i32.load offset=28
            i32.load offset=16
            call_indirect $13 (type $2)
            i32.eqz
            br_if $loop_0
          end ;; $loop_0
          i32.const 1
          return
        end ;; $block_6
        i32.const 1
        local.set $10
        local.get $0
        i32.const 1
        i32.store8 offset=48
        local.get $0
        i32.const 48
        i32.store offset=4
        local.get $0
        local.get $8
        local.get $1
        local.get $2
        call $core::fmt::Formatter::pad_integral::write_prefix::h9e67c72a92bd1433
        br_if $block_2
        local.get $9
        local.get $7
        i32.sub
        local.set $8
        i32.const 0
        local.set $10
        block $block_10
          block $block_11
            block $block_12
              i32.const 1
              local.get $0
              i32.load8_u offset=48
              local.tee $9
              local.get $9
              i32.const 3
              i32.eq
              select
              br_table
                $block_10 $block_12 $block_11 $block_12
                $block_10 ;; default
            end ;; $block_12
            local.get $8
            local.set $10
            i32.const 0
            local.set $8
            br $block_10
          end ;; $block_11
          local.get $8
          i32.const 1
          i32.shr_u
          local.set $10
          local.get $8
          i32.const 1
          i32.add
          i32.const 1
          i32.shr_u
          local.set $8
        end ;; $block_10
        local.get $10
        i32.const 1
        i32.add
        local.set $10
        block $block_13
          loop $loop_1
            local.get $10
            i32.const -1
            i32.add
            local.tee $10
            i32.eqz
            br_if $block_13
            local.get $0
            i32.load offset=24
            local.get $0
            i32.load offset=4
            local.get $0
            i32.load offset=28
            i32.load offset=16
            call_indirect $13 (type $2)
            i32.eqz
            br_if $loop_1
          end ;; $loop_1
          i32.const 1
          return
        end ;; $block_13
        local.get $0
        i32.load offset=4
        local.set $9
        i32.const 1
        local.set $10
        local.get $0
        i32.load offset=24
        local.get $3
        local.get $4
        local.get $0
        i32.load offset=28
        i32.load offset=12
        call_indirect $13 (type $1)
        br_if $block_2
        local.get $8
        i32.const 1
        i32.add
        local.set $8
        local.get $0
        i32.load offset=28
        local.set $2
        local.get $0
        i32.load offset=24
        local.set $0
        loop $loop_2
          block $block_14
            local.get $8
            i32.const -1
            i32.add
            local.tee $8
            br_if $block_14
            i32.const 0
            return
          end ;; $block_14
          i32.const 1
          local.set $10
          local.get $0
          local.get $9
          local.get $2
          i32.load offset=16
          call_indirect $13 (type $2)
          i32.eqz
          br_if $loop_2
          br $block_2
        end ;; $loop_2
      end ;; $block_5
      local.get $0
      i32.load offset=4
      local.set $7
      i32.const 1
      local.set $10
      local.get $0
      local.get $8
      local.get $1
      local.get $2
      call $core::fmt::Formatter::pad_integral::write_prefix::h9e67c72a92bd1433
      br_if $block_2
      local.get $0
      i32.load offset=24
      local.get $3
      local.get $4
      local.get $0
      i32.load offset=28
      i32.load offset=12
      call_indirect $13 (type $1)
      br_if $block_2
      local.get $9
      i32.const 1
      i32.add
      local.set $8
      local.get $0
      i32.load offset=28
      local.set $9
      local.get $0
      i32.load offset=24
      local.set $0
      loop $loop_3
        block $block_15
          local.get $8
          i32.const -1
          i32.add
          local.tee $8
          br_if $block_15
          i32.const 0
          return
        end ;; $block_15
        i32.const 1
        local.set $10
        local.get $0
        local.get $7
        local.get $9
        i32.load offset=16
        call_indirect $13 (type $2)
        i32.eqz
        br_if $loop_3
      end ;; $loop_3
    end ;; $block_2
    local.get $10
    )
  
  (func $core::fmt::Formatter::pad_integral::write_prefix::h9e67c72a92bd1433 (type $12)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (result i32)
    (local $4 i32)
    block $block
      block $block_0
        local.get $1
        i32.const 1114112
        i32.eq
        br_if $block_0
        i32.const 1
        local.set $4
        local.get $0
        i32.load offset=24
        local.get $1
        local.get $0
        i32.const 28
        i32.add
        i32.load
        i32.load offset=16
        call_indirect $13 (type $2)
        br_if $block
      end ;; $block_0
      block $block_1
        local.get $2
        br_if $block_1
        i32.const 0
        return
      end ;; $block_1
      local.get $0
      i32.load offset=24
      local.get $2
      local.get $3
      local.get $0
      i32.const 28
      i32.add
      i32.load
      i32.load offset=12
      call_indirect $13 (type $1)
      local.set $4
    end ;; $block
    local.get $4
    )
  
  (func $core::slice::slice_index_order_fail::h52dc23ecec067d2f (type $0)
    (param $0 i32)
    (param $1 i32)
    (local $2 i32)
    global.get $15
    i32.const 48
    i32.sub
    local.tee $2
    global.set $15
    local.get $2
    local.get $1
    i32.store offset=4
    local.get $2
    local.get $0
    i32.store
    local.get $2
    i32.const 28
    i32.add
    i32.const 2
    i32.store
    local.get $2
    i32.const 44
    i32.add
    i32.const 6
    i32.store
    local.get $2
    i64.const 2
    i64.store offset=12 align=4
    local.get $2
    i32.const 1049480
    i32.store offset=8
    local.get $2
    i32.const 6
    i32.store offset=36
    local.get $2
    local.get $2
    i32.const 32
    i32.add
    i32.store offset=24
    local.get $2
    local.get $2
    i32.const 4
    i32.add
    i32.store offset=40
    local.get $2
    local.get $2
    i32.store offset=32
    local.get $2
    i32.const 8
    i32.add
    i32.const 1049496
    call $core::panicking::panic_fmt::h772cfe55da576359
    unreachable
    )
  
  (func $core::str::slice_error_fail::he0fc563abf615243 (type $7)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    (local $7 i32)
    (local $8 i32)
    (local $9 i32)
    global.get $15
    i32.const 112
    i32.sub
    local.tee $4
    global.set $15
    local.get $4
    local.get $3
    i32.store offset=12
    local.get $4
    local.get $2
    i32.store offset=8
    i32.const 1
    local.set $5
    local.get $1
    local.set $6
    block $block
      local.get $1
      i32.const 257
      i32.lt_u
      br_if $block
      i32.const 0
      local.get $1
      i32.sub
      local.set $7
      i32.const 256
      local.set $8
      loop $loop
        block $block_0
          local.get $8
          local.get $1
          i32.ge_u
          br_if $block_0
          local.get $0
          local.get $8
          i32.add
          i32.load8_s
          i32.const -65
          i32.le_s
          br_if $block_0
          i32.const 0
          local.set $5
          local.get $8
          local.set $6
          br $block
        end ;; $block_0
        local.get $8
        i32.const -1
        i32.add
        local.set $6
        i32.const 0
        local.set $5
        local.get $8
        i32.const 1
        i32.eq
        br_if $block
        local.get $7
        local.get $8
        i32.add
        local.set $9
        local.get $6
        local.set $8
        local.get $9
        i32.const 1
        i32.ne
        br_if $loop
      end ;; $loop
    end ;; $block
    local.get $4
    local.get $6
    i32.store offset=20
    local.get $4
    local.get $0
    i32.store offset=16
    local.get $4
    i32.const 0
    i32.const 5
    local.get $5
    select
    i32.store offset=28
    local.get $4
    i32.const 1049116
    i32.const 1049550
    local.get $5
    select
    i32.store offset=24
    block $block_1
      block $block_2
        block $block_3
          block $block_4
            local.get $2
            local.get $1
            i32.gt_u
            local.tee $8
            br_if $block_4
            local.get $3
            local.get $1
            i32.gt_u
            br_if $block_4
            local.get $2
            local.get $3
            i32.gt_u
            br_if $block_3
            block $block_5
              block $block_6
                local.get $2
                i32.eqz
                br_if $block_6
                local.get $1
                local.get $2
                i32.eq
                br_if $block_6
                local.get $1
                local.get $2
                i32.le_u
                br_if $block_5
                local.get $0
                local.get $2
                i32.add
                i32.load8_s
                i32.const -64
                i32.lt_s
                br_if $block_5
              end ;; $block_6
              local.get $3
              local.set $2
            end ;; $block_5
            local.get $4
            local.get $2
            i32.store offset=32
            local.get $2
            i32.eqz
            br_if $block_2
            local.get $2
            local.get $1
            i32.eq
            br_if $block_2
            local.get $1
            i32.const 1
            i32.add
            local.set $9
            loop $loop_0
              block $block_7
                local.get $2
                local.get $1
                i32.ge_u
                br_if $block_7
                local.get $0
                local.get $2
                i32.add
                i32.load8_s
                i32.const -64
                i32.ge_s
                br_if $block_2
              end ;; $block_7
              local.get $2
              i32.const -1
              i32.add
              local.set $8
              local.get $2
              i32.const 1
              i32.eq
              br_if $block_1
              local.get $9
              local.get $2
              i32.eq
              local.set $6
              local.get $8
              local.set $2
              local.get $6
              i32.eqz
              br_if $loop_0
              br $block_1
            end ;; $loop_0
          end ;; $block_4
          local.get $4
          local.get $2
          local.get $3
          local.get $8
          select
          i32.store offset=40
          local.get $4
          i32.const 48
          i32.add
          i32.const 20
          i32.add
          i32.const 3
          i32.store
          local.get $4
          i32.const 72
          i32.add
          i32.const 20
          i32.add
          i32.const 1
          i32.store
          local.get $4
          i32.const 84
          i32.add
          i32.const 1
          i32.store
          local.get $4
          i64.const 3
          i64.store offset=52 align=4
          local.get $4
          i32.const 1049556
          i32.store offset=48
          local.get $4
          i32.const 6
          i32.store offset=76
          local.get $4
          local.get $4
          i32.const 72
          i32.add
          i32.store offset=64
          local.get $4
          local.get $4
          i32.const 24
          i32.add
          i32.store offset=88
          local.get $4
          local.get $4
          i32.const 16
          i32.add
          i32.store offset=80
          local.get $4
          local.get $4
          i32.const 40
          i32.add
          i32.store offset=72
          local.get $4
          i32.const 48
          i32.add
          i32.const 1049580
          call $core::panicking::panic_fmt::h772cfe55da576359
          unreachable
        end ;; $block_3
        local.get $4
        i32.const 100
        i32.add
        i32.const 1
        i32.store
        local.get $4
        i32.const 72
        i32.add
        i32.const 20
        i32.add
        i32.const 1
        i32.store
        local.get $4
        i32.const 84
        i32.add
        i32.const 6
        i32.store
        local.get $4
        i32.const 48
        i32.add
        i32.const 20
        i32.add
        i32.const 4
        i32.store
        local.get $4
        i64.const 4
        i64.store offset=52 align=4
        local.get $4
        i32.const 1049596
        i32.store offset=48
        local.get $4
        i32.const 6
        i32.store offset=76
        local.get $4
        local.get $4
        i32.const 72
        i32.add
        i32.store offset=64
        local.get $4
        local.get $4
        i32.const 24
        i32.add
        i32.store offset=96
        local.get $4
        local.get $4
        i32.const 16
        i32.add
        i32.store offset=88
        local.get $4
        local.get $4
        i32.const 12
        i32.add
        i32.store offset=80
        local.get $4
        local.get $4
        i32.const 8
        i32.add
        i32.store offset=72
        local.get $4
        i32.const 48
        i32.add
        i32.const 1049628
        call $core::panicking::panic_fmt::h772cfe55da576359
        unreachable
      end ;; $block_2
      local.get $2
      local.set $8
    end ;; $block_1
    block $block_8
      local.get $8
      local.get $1
      i32.eq
      br_if $block_8
      i32.const 1
      local.set $6
      block $block_9
        block $block_10
          block $block_11
            block $block_12
              local.get $0
              local.get $8
              i32.add
              local.tee $9
              i32.load8_s
              local.tee $2
              i32.const -1
              i32.gt_s
              br_if $block_12
              i32.const 0
              local.set $5
              local.get $0
              local.get $1
              i32.add
              local.tee $6
              local.set $1
              block $block_13
                local.get $9
                i32.const 1
                i32.add
                local.get $6
                i32.eq
                br_if $block_13
                local.get $9
                i32.const 2
                i32.add
                local.set $1
                local.get $9
                i32.load8_u offset=1
                i32.const 63
                i32.and
                local.set $5
              end ;; $block_13
              local.get $2
              i32.const 31
              i32.and
              local.set $9
              local.get $2
              i32.const 255
              i32.and
              i32.const 223
              i32.gt_u
              br_if $block_11
              local.get $5
              local.get $9
              i32.const 6
              i32.shl
              i32.or
              local.set $1
              br $block_10
            end ;; $block_12
            local.get $4
            local.get $2
            i32.const 255
            i32.and
            i32.store offset=36
            local.get $4
            i32.const 40
            i32.add
            local.set $2
            br $block_9
          end ;; $block_11
          i32.const 0
          local.set $0
          local.get $6
          local.set $7
          block $block_14
            local.get $1
            local.get $6
            i32.eq
            br_if $block_14
            local.get $1
            i32.const 1
            i32.add
            local.set $7
            local.get $1
            i32.load8_u
            i32.const 63
            i32.and
            local.set $0
          end ;; $block_14
          local.get $0
          local.get $5
          i32.const 6
          i32.shl
          i32.or
          local.set $1
          block $block_15
            local.get $2
            i32.const 255
            i32.and
            i32.const 240
            i32.ge_u
            br_if $block_15
            local.get $1
            local.get $9
            i32.const 12
            i32.shl
            i32.or
            local.set $1
            br $block_10
          end ;; $block_15
          i32.const 0
          local.set $2
          block $block_16
            local.get $7
            local.get $6
            i32.eq
            br_if $block_16
            local.get $7
            i32.load8_u
            i32.const 63
            i32.and
            local.set $2
          end ;; $block_16
          local.get $1
          i32.const 6
          i32.shl
          local.get $9
          i32.const 18
          i32.shl
          i32.const 1835008
          i32.and
          i32.or
          local.get $2
          i32.or
          local.tee $1
          i32.const 1114112
          i32.eq
          br_if $block_8
        end ;; $block_10
        local.get $4
        local.get $1
        i32.store offset=36
        i32.const 1
        local.set $6
        local.get $4
        i32.const 40
        i32.add
        local.set $2
        local.get $1
        i32.const 128
        i32.lt_u
        br_if $block_9
        i32.const 2
        local.set $6
        local.get $1
        i32.const 2048
        i32.lt_u
        br_if $block_9
        i32.const 3
        i32.const 4
        local.get $1
        i32.const 65536
        i32.lt_u
        select
        local.set $6
      end ;; $block_9
      local.get $4
      local.get $8
      i32.store offset=40
      local.get $4
      local.get $6
      local.get $8
      i32.add
      i32.store offset=44
      local.get $4
      i32.const 48
      i32.add
      i32.const 20
      i32.add
      i32.const 5
      i32.store
      local.get $4
      i32.const 108
      i32.add
      i32.const 1
      i32.store
      local.get $4
      i32.const 100
      i32.add
      i32.const 1
      i32.store
      local.get $4
      i32.const 72
      i32.add
      i32.const 20
      i32.add
      i32.const 7
      i32.store
      local.get $4
      i32.const 84
      i32.add
      i32.const 8
      i32.store
      local.get $4
      i64.const 5
      i64.store offset=52 align=4
      local.get $4
      i32.const 1049668
      i32.store offset=48
      local.get $4
      local.get $2
      i32.store offset=88
      local.get $4
      i32.const 6
      i32.store offset=76
      local.get $4
      local.get $4
      i32.const 72
      i32.add
      i32.store offset=64
      local.get $4
      local.get $4
      i32.const 24
      i32.add
      i32.store offset=104
      local.get $4
      local.get $4
      i32.const 16
      i32.add
      i32.store offset=96
      local.get $4
      local.get $4
      i32.const 36
      i32.add
      i32.store offset=80
      local.get $4
      local.get $4
      i32.const 32
      i32.add
      i32.store offset=72
      local.get $4
      i32.const 48
      i32.add
      i32.const 1049708
      call $core::panicking::panic_fmt::h772cfe55da576359
      unreachable
    end ;; $block_8
    i32.const 1049644
    call $core::panicking::panic::h540eb5ee6ff533c2
    unreachable
    )
  
  (func $<core::ops::range::Range<Idx>_as_core::fmt::Debug>::fmt::haf7bc23b95386435 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    global.get $15
    i32.const 32
    i32.sub
    local.tee $2
    global.set $15
    block $block
      local.get $0
      i32.load
      local.get $1
      call $core::fmt::num::<impl_core::fmt::Debug_for_usize>::fmt::hb54eecdb0d5e8e4a
      br_if $block
      local.get $1
      i32.const 28
      i32.add
      i32.load
      local.set $3
      local.get $1
      i32.load offset=24
      local.set $4
      local.get $2
      i32.const 28
      i32.add
      i32.const 0
      i32.store
      local.get $2
      i32.const 1049116
      i32.store offset=24
      local.get $2
      i64.const 1
      i64.store offset=12 align=4
      local.get $2
      i32.const 1049812
      i32.store offset=8
      local.get $4
      local.get $3
      local.get $2
      i32.const 8
      i32.add
      call $core::fmt::write::h9ffb25a5a03fc281
      br_if $block
      local.get $0
      i32.load offset=4
      local.get $1
      call $core::fmt::num::<impl_core::fmt::Debug_for_usize>::fmt::hb54eecdb0d5e8e4a
      local.set $1
      local.get $2
      i32.const 32
      i32.add
      global.set $15
      local.get $1
      return
    end ;; $block
    local.get $2
    i32.const 32
    i32.add
    global.set $15
    i32.const 1
    )
  
  (func $<char_as_core::fmt::Debug>::fmt::h27b79e1831e51870 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    (local $7 i32)
    (local $8 i32)
    (local $9 i64)
    i32.const 1
    local.set $2
    block $block
      local.get $1
      i32.load offset=24
      i32.const 39
      local.get $1
      i32.const 28
      i32.add
      i32.load
      i32.load offset=16
      call_indirect $13 (type $2)
      br_if $block
      i32.const 2
      local.set $3
      block $block_0
        block $block_1
          block $block_2
            block $block_3
              block $block_4
                local.get $0
                i32.load
                local.tee $0
                i32.const -9
                i32.add
                local.tee $4
                i32.const 30
                i32.le_u
                br_if $block_4
                local.get $0
                i32.const 92
                i32.ne
                br_if $block_3
                br $block_2
              end ;; $block_4
              i32.const 116
              local.set $5
              block $block_5
                block $block_6
                  local.get $4
                  br_table
                    $block_0 $block_5 $block_3 $block_3 $block_6 $block_3 $block_3 $block_3 $block_3 $block_3 $block_3 $block_3 $block_3 $block_3 $block_3 $block_3
                    $block_3 $block_3 $block_3 $block_3 $block_3 $block_3 $block_3 $block_3 $block_3 $block_2 $block_3 $block_3 $block_3 $block_3 $block_2
                    $block_0 ;; default
                end ;; $block_6
                i32.const 114
                local.set $5
                br $block_0
              end ;; $block_5
              i32.const 110
              local.set $5
              br $block_0
            end ;; $block_3
            block $block_7
              block $block_8
                local.get $0
                i32.const 2048
                i32.lt_u
                br_if $block_8
                block $block_9
                  block $block_10
                    block $block_11
                      block $block_12
                        block $block_13
                          block $block_14
                            local.get $0
                            i32.const 65536
                            i32.lt_u
                            br_if $block_14
                            local.get $0
                            i32.const 12
                            i32.shr_u
                            i32.const -16
                            i32.add
                            local.tee $3
                            i32.const 256
                            i32.lt_u
                            br_if $block_13
                            i32.const 1052776
                            local.get $3
                            i32.const 256
                            call $core::panicking::panic_bounds_check::hb65bbcdf415380e4
                            unreachable
                          end ;; $block_14
                          local.get $0
                          i32.const 6
                          i32.shr_u
                          i32.const -32
                          i32.add
                          local.tee $3
                          i32.const 991
                          i32.gt_u
                          br_if $block_12
                          local.get $3
                          i32.const 1050160
                          i32.add
                          i32.load8_u
                          local.tee $3
                          i32.const 73
                          i32.gt_u
                          br_if $block_11
                          local.get $3
                          i32.const 3
                          i32.shl
                          i32.const 1052864
                          i32.add
                          local.set $3
                          br $block_7
                        end ;; $block_13
                        local.get $3
                        i32.const 1051152
                        i32.add
                        i32.load8_u
                        i32.const 6
                        i32.shl
                        local.get $0
                        i32.const 6
                        i32.shr_u
                        i32.const 63
                        i32.and
                        i32.or
                        local.tee $3
                        i32.const 511
                        i32.gt_u
                        br_if $block_10
                        local.get $3
                        i32.const 1053456
                        i32.add
                        i32.load8_u
                        local.tee $3
                        i32.const 57
                        i32.gt_u
                        br_if $block_9
                        local.get $3
                        i32.const 3
                        i32.shl
                        i32.const 1053968
                        i32.add
                        local.set $3
                        br $block_7
                      end ;; $block_12
                      i32.const 1052744
                      local.get $3
                      i32.const 992
                      call $core::panicking::panic_bounds_check::hb65bbcdf415380e4
                      unreachable
                    end ;; $block_11
                    i32.const 1052760
                    local.get $3
                    i32.const 74
                    call $core::panicking::panic_bounds_check::hb65bbcdf415380e4
                    unreachable
                  end ;; $block_10
                  i32.const 1052792
                  local.get $3
                  i32.const 512
                  call $core::panicking::panic_bounds_check::hb65bbcdf415380e4
                  unreachable
                end ;; $block_9
                i32.const 1052808
                local.get $3
                i32.const 58
                call $core::panicking::panic_bounds_check::hb65bbcdf415380e4
                unreachable
              end ;; $block_8
              local.get $0
              i32.const 3
              i32.shr_u
              i32.const 262136
              i32.and
              i32.const 1049880
              i32.add
              local.set $3
            end ;; $block_7
            block $block_15
              block $block_16
                block $block_17
                  block $block_18
                    block $block_19
                      block $block_20
                        block $block_21
                          local.get $3
                          i64.load
                          i64.const 1
                          local.get $0
                          i32.const 63
                          i32.and
                          i64.extend_i32_u
                          i64.shl
                          i64.and
                          i64.const 0
                          i64.ne
                          br_if $block_21
                          block $block_22
                            block $block_23
                              block $block_24
                                block $block_25
                                  block $block_26
                                    block $block_27
                                      local.get $0
                                      i32.const 65536
                                      i32.lt_u
                                      br_if $block_27
                                      local.get $0
                                      i32.const 131072
                                      i32.lt_u
                                      br_if $block_26
                                      local.get $0
                                      i32.const -918000
                                      i32.add
                                      i32.const 196112
                                      i32.lt_u
                                      br_if $block_17
                                      local.get $0
                                      i32.const -195102
                                      i32.add
                                      i32.const 722658
                                      i32.lt_u
                                      br_if $block_17
                                      local.get $0
                                      i32.const -191457
                                      i32.add
                                      i32.const 3103
                                      i32.lt_u
                                      br_if $block_17
                                      local.get $0
                                      i32.const -183970
                                      i32.add
                                      i32.const 14
                                      i32.lt_u
                                      br_if $block_17
                                      local.get $0
                                      i32.const 2097150
                                      i32.and
                                      i32.const 178206
                                      i32.eq
                                      br_if $block_17
                                      local.get $0
                                      i32.const -173783
                                      i32.add
                                      i32.const 41
                                      i32.lt_u
                                      br_if $block_17
                                      local.get $0
                                      i32.const -177973
                                      i32.add
                                      i32.const 10
                                      i32.le_u
                                      br_if $block_17
                                      i32.const 1
                                      local.set $3
                                      br $block_1
                                    end ;; $block_27
                                    local.get $0
                                    i32.const 65280
                                    i32.and
                                    i32.const 8
                                    i32.shr_u
                                    local.set $6
                                    i32.const 1051408
                                    local.set $4
                                    i32.const 0
                                    local.set $2
                                    local.get $0
                                    i32.const 255
                                    i32.and
                                    local.set $5
                                    loop $loop
                                      local.get $4
                                      i32.const 2
                                      i32.add
                                      local.set $7
                                      local.get $2
                                      local.get $4
                                      i32.load8_u offset=1
                                      local.tee $3
                                      i32.add
                                      local.set $8
                                      block $block_28
                                        local.get $4
                                        i32.load8_u
                                        local.tee $4
                                        local.get $6
                                        i32.eq
                                        br_if $block_28
                                        local.get $4
                                        local.get $6
                                        i32.gt_u
                                        br_if $block_19
                                        local.get $8
                                        local.set $2
                                        local.get $7
                                        local.set $4
                                        local.get $7
                                        i32.const 1051490
                                        i32.ne
                                        br_if $loop
                                        br $block_19
                                      end ;; $block_28
                                      local.get $8
                                      local.get $2
                                      i32.lt_u
                                      br_if $block_25
                                      local.get $8
                                      i32.const 293
                                      i32.gt_u
                                      br_if $block_24
                                      local.get $2
                                      i32.const 1051490
                                      i32.add
                                      local.set $4
                                      block $block_29
                                        loop $loop_0
                                          local.get $3
                                          i32.eqz
                                          br_if $block_29
                                          local.get $3
                                          i32.const -1
                                          i32.add
                                          local.set $3
                                          local.get $4
                                          i32.load8_u
                                          local.set $2
                                          local.get $4
                                          i32.const 1
                                          i32.add
                                          local.set $4
                                          local.get $2
                                          local.get $5
                                          i32.ne
                                          br_if $loop_0
                                          br $block_17
                                        end ;; $loop_0
                                      end ;; $block_29
                                      local.get $8
                                      local.set $2
                                      local.get $7
                                      local.set $4
                                      local.get $7
                                      i32.const 1051490
                                      i32.ne
                                      br_if $loop
                                      br $block_19
                                    end ;; $loop
                                  end ;; $block_26
                                  local.get $0
                                  i32.const 65280
                                  i32.and
                                  i32.const 8
                                  i32.shr_u
                                  local.set $6
                                  i32.const 1052097
                                  local.set $4
                                  i32.const 0
                                  local.set $2
                                  local.get $0
                                  i32.const 255
                                  i32.and
                                  local.set $5
                                  loop $loop_1
                                    local.get $4
                                    i32.const 2
                                    i32.add
                                    local.set $7
                                    local.get $2
                                    local.get $4
                                    i32.load8_u offset=1
                                    local.tee $3
                                    i32.add
                                    local.set $8
                                    block $block_30
                                      local.get $4
                                      i32.load8_u
                                      local.tee $4
                                      local.get $6
                                      i32.eq
                                      br_if $block_30
                                      local.get $4
                                      local.get $6
                                      i32.gt_u
                                      br_if $block_20
                                      local.get $8
                                      local.set $2
                                      local.get $7
                                      local.set $4
                                      local.get $7
                                      i32.const 1052167
                                      i32.ne
                                      br_if $loop_1
                                      br $block_20
                                    end ;; $block_30
                                    local.get $8
                                    local.get $2
                                    i32.lt_u
                                    br_if $block_23
                                    local.get $8
                                    i32.const 166
                                    i32.gt_u
                                    br_if $block_22
                                    local.get $2
                                    i32.const 1052167
                                    i32.add
                                    local.set $4
                                    block $block_31
                                      loop $loop_2
                                        local.get $3
                                        i32.eqz
                                        br_if $block_31
                                        local.get $3
                                        i32.const -1
                                        i32.add
                                        local.set $3
                                        local.get $4
                                        i32.load8_u
                                        local.set $2
                                        local.get $4
                                        i32.const 1
                                        i32.add
                                        local.set $4
                                        local.get $2
                                        local.get $5
                                        i32.ne
                                        br_if $loop_2
                                        br $block_17
                                      end ;; $loop_2
                                    end ;; $block_31
                                    local.get $8
                                    local.set $2
                                    local.get $7
                                    local.set $4
                                    local.get $7
                                    i32.const 1052167
                                    i32.ne
                                    br_if $loop_1
                                    br $block_20
                                  end ;; $loop_1
                                end ;; $block_25
                                local.get $2
                                local.get $8
                                call $core::slice::slice_index_order_fail::h52dc23ecec067d2f
                                unreachable
                              end ;; $block_24
                              local.get $8
                              i32.const 293
                              call $core::slice::slice_index_len_fail::h7c242876dcc4f7b2
                              unreachable
                            end ;; $block_23
                            local.get $2
                            local.get $8
                            call $core::slice::slice_index_order_fail::h52dc23ecec067d2f
                            unreachable
                          end ;; $block_22
                          local.get $8
                          i32.const 166
                          call $core::slice::slice_index_len_fail::h7c242876dcc4f7b2
                          unreachable
                        end ;; $block_21
                        local.get $0
                        i32.const 1
                        i32.or
                        i32.clz
                        i32.const 2
                        i32.shr_u
                        i32.const 7
                        i32.xor
                        i64.extend_i32_u
                        i64.const 21474836480
                        i64.or
                        local.set $9
                        br $block_16
                      end ;; $block_20
                      local.get $0
                      i32.const 65535
                      i32.and
                      local.set $5
                      i32.const 1052333
                      local.set $3
                      i32.const 1
                      local.set $4
                      block $block_32
                        loop $loop_3
                          local.get $3
                          i32.const 1
                          i32.add
                          local.set $8
                          block $block_33
                            block $block_34
                              local.get $3
                              i32.load8_u
                              local.tee $2
                              i32.const 24
                              i32.shl
                              i32.const 24
                              i32.shr_s
                              local.tee $7
                              i32.const 0
                              i32.lt_s
                              br_if $block_34
                              local.get $8
                              local.set $3
                              br $block_33
                            end ;; $block_34
                            local.get $8
                            i32.const 1052741
                            i32.eq
                            br_if $block_32
                            local.get $7
                            i32.const 127
                            i32.and
                            i32.const 8
                            i32.shl
                            local.get $3
                            i32.load8_u offset=1
                            i32.or
                            local.set $2
                            local.get $3
                            i32.const 2
                            i32.add
                            local.set $3
                          end ;; $block_33
                          local.get $5
                          local.get $2
                          i32.sub
                          local.tee $5
                          i32.const 0
                          i32.lt_s
                          br_if $block_18
                          local.get $4
                          i32.const 1
                          i32.xor
                          local.set $4
                          local.get $3
                          i32.const 1052741
                          i32.ne
                          br_if $loop_3
                          br $block_18
                        end ;; $loop_3
                      end ;; $block_32
                      i32.const 1049644
                      call $core::panicking::panic::h540eb5ee6ff533c2
                      unreachable
                    end ;; $block_19
                    local.get $0
                    i32.const 65535
                    i32.and
                    local.set $5
                    i32.const 1051783
                    local.set $3
                    i32.const 1
                    local.set $4
                    loop $loop_4
                      local.get $3
                      i32.const 1
                      i32.add
                      local.set $8
                      block $block_35
                        block $block_36
                          local.get $3
                          i32.load8_u
                          local.tee $2
                          i32.const 24
                          i32.shl
                          i32.const 24
                          i32.shr_s
                          local.tee $7
                          i32.const 0
                          i32.lt_s
                          br_if $block_36
                          local.get $8
                          local.set $3
                          br $block_35
                        end ;; $block_36
                        local.get $8
                        i32.const 1052097
                        i32.eq
                        br_if $block_15
                        local.get $7
                        i32.const 127
                        i32.and
                        i32.const 8
                        i32.shl
                        local.get $3
                        i32.load8_u offset=1
                        i32.or
                        local.set $2
                        local.get $3
                        i32.const 2
                        i32.add
                        local.set $3
                      end ;; $block_35
                      local.get $5
                      local.get $2
                      i32.sub
                      local.tee $5
                      i32.const 0
                      i32.lt_s
                      br_if $block_18
                      local.get $4
                      i32.const 1
                      i32.xor
                      local.set $4
                      local.get $3
                      i32.const 1052097
                      i32.ne
                      br_if $loop_4
                    end ;; $loop_4
                  end ;; $block_18
                  i32.const 1
                  local.set $3
                  local.get $4
                  i32.const 1
                  i32.and
                  br_if $block_2
                end ;; $block_17
                local.get $0
                i32.const 1
                i32.or
                i32.clz
                i32.const 2
                i32.shr_u
                i32.const 7
                i32.xor
                i64.extend_i32_u
                i64.const 21474836480
                i64.or
                local.set $9
              end ;; $block_16
              i32.const 3
              local.set $3
              br $block_1
            end ;; $block_15
            i32.const 1049644
            call $core::panicking::panic::h540eb5ee6ff533c2
            unreachable
          end ;; $block_2
        end ;; $block_1
        local.get $0
        local.set $5
      end ;; $block_0
      loop $loop_5
        local.get $3
        local.set $4
        i32.const 92
        local.set $0
        i32.const 1
        local.set $2
        i32.const 1
        local.set $3
        block $block_37
          block $block_38
            block $block_39
              block $block_40
                local.get $4
                br_table
                  $block_39 $block_38 $block_37 $block_40
                  $block_39 ;; default
              end ;; $block_40
              block $block_41
                block $block_42
                  block $block_43
                    block $block_44
                      block $block_45
                        local.get $9
                        i64.const 32
                        i64.shr_u
                        i32.wrap_i64
                        i32.const 255
                        i32.and
                        br_table
                          $block_39 $block_41 $block_42 $block_43 $block_44 $block_45
                          $block_39 ;; default
                      end ;; $block_45
                      local.get $9
                      i64.const -1095216660481
                      i64.and
                      i64.const 17179869184
                      i64.or
                      local.set $9
                      i32.const 3
                      local.set $3
                      br $block_37
                    end ;; $block_44
                    local.get $9
                    i64.const -1095216660481
                    i64.and
                    i64.const 12884901888
                    i64.or
                    local.set $9
                    i32.const 117
                    local.set $0
                    i32.const 3
                    local.set $3
                    br $block_37
                  end ;; $block_43
                  local.get $9
                  i64.const -1095216660481
                  i64.and
                  i64.const 8589934592
                  i64.or
                  local.set $9
                  i32.const 123
                  local.set $0
                  i32.const 3
                  local.set $3
                  br $block_37
                end ;; $block_42
                local.get $5
                local.get $9
                i32.wrap_i64
                local.tee $4
                i32.const 2
                i32.shl
                i32.const 28
                i32.and
                i32.shr_u
                i32.const 15
                i32.and
                local.tee $3
                i32.const 48
                i32.or
                local.get $3
                i32.const 87
                i32.add
                local.get $3
                i32.const 10
                i32.lt_u
                select
                local.set $0
                block $block_46
                  local.get $4
                  i32.eqz
                  br_if $block_46
                  local.get $9
                  i64.const -1
                  i64.add
                  i64.const 4294967295
                  i64.and
                  local.get $9
                  i64.const -4294967296
                  i64.and
                  i64.or
                  local.set $9
                  i32.const 3
                  local.set $3
                  br $block_37
                end ;; $block_46
                local.get $9
                i64.const -1095216660481
                i64.and
                i64.const 4294967296
                i64.or
                local.set $9
                i32.const 3
                local.set $3
                br $block_37
              end ;; $block_41
              local.get $9
              i64.const -1095216660481
              i64.and
              local.set $9
              i32.const 125
              local.set $0
              i32.const 3
              local.set $3
              br $block_37
            end ;; $block_39
            local.get $1
            i32.load offset=24
            i32.const 39
            local.get $1
            i32.load offset=28
            i32.load offset=16
            call_indirect $13 (type $2)
            return
          end ;; $block_38
          i32.const 0
          local.set $3
          local.get $5
          local.set $0
        end ;; $block_37
        local.get $1
        i32.load offset=24
        local.get $0
        local.get $1
        i32.load offset=28
        i32.load offset=16
        call_indirect $13 (type $2)
        i32.eqz
        br_if $loop_5
      end ;; $loop_5
    end ;; $block
    local.get $2
    )
  
  (func $core::fmt::num::<impl_core::fmt::Debug_for_usize>::fmt::hb54eecdb0d5e8e4a (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    global.get $15
    i32.const 128
    i32.sub
    local.tee $2
    global.set $15
    block $block
      block $block_0
        block $block_1
          block $block_2
            block $block_3
              local.get $1
              i32.load
              local.tee $3
              i32.const 16
              i32.and
              br_if $block_3
              local.get $3
              i32.const 32
              i32.and
              br_if $block_2
              local.get $0
              i64.extend_i32_u
              local.get $1
              call $core::fmt::num::imp::fmt_u64::hbf92ea5b56adc693
              local.set $0
              br $block_1
            end ;; $block_3
            i32.const 0
            local.set $3
            loop $loop
              local.get $2
              local.get $3
              i32.add
              i32.const 127
              i32.add
              local.get $0
              i32.const 15
              i32.and
              local.tee $4
              i32.const 48
              i32.or
              local.get $4
              i32.const 87
              i32.add
              local.get $4
              i32.const 10
              i32.lt_u
              select
              i32.store8
              local.get $3
              i32.const -1
              i32.add
              local.set $3
              local.get $0
              i32.const 4
              i32.shr_u
              local.tee $0
              br_if $loop
            end ;; $loop
            local.get $3
            i32.const 128
            i32.add
            local.tee $0
            i32.const 129
            i32.ge_u
            br_if $block_0
            local.get $1
            i32.const 1049548
            i32.const 2
            local.get $2
            local.get $3
            i32.add
            i32.const 128
            i32.add
            i32.const 0
            local.get $3
            i32.sub
            call $core::fmt::Formatter::pad_integral::h0e630a1dafcc54e1
            local.set $0
            br $block_1
          end ;; $block_2
          i32.const 0
          local.set $3
          loop $loop_0
            local.get $2
            local.get $3
            i32.add
            i32.const 127
            i32.add
            local.get $0
            i32.const 15
            i32.and
            local.tee $4
            i32.const 48
            i32.or
            local.get $4
            i32.const 55
            i32.add
            local.get $4
            i32.const 10
            i32.lt_u
            select
            i32.store8
            local.get $3
            i32.const -1
            i32.add
            local.set $3
            local.get $0
            i32.const 4
            i32.shr_u
            local.tee $0
            br_if $loop_0
          end ;; $loop_0
          local.get $3
          i32.const 128
          i32.add
          local.tee $0
          i32.const 129
          i32.ge_u
          br_if $block
          local.get $1
          i32.const 1049548
          i32.const 2
          local.get $2
          local.get $3
          i32.add
          i32.const 128
          i32.add
          i32.const 0
          local.get $3
          i32.sub
          call $core::fmt::Formatter::pad_integral::h0e630a1dafcc54e1
          local.set $0
        end ;; $block_1
        local.get $2
        i32.const 128
        i32.add
        global.set $15
        local.get $0
        return
      end ;; $block_0
      local.get $0
      i32.const 128
      call $core::slice::slice_index_order_fail::h52dc23ecec067d2f
      unreachable
    end ;; $block
    local.get $0
    i32.const 128
    call $core::slice::slice_index_order_fail::h52dc23ecec067d2f
    unreachable
    )
  
  (func $core::fmt::ArgumentV1::show_usize::h42d8eed9ea762fba (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    local.get $0
    i64.load32_u
    local.get $1
    call $core::fmt::num::imp::fmt_u64::hbf92ea5b56adc693
    )
  
  (func $<&T_as_core::fmt::Debug>::fmt::h71574e0037640cf9 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    local.get $0
    i32.load
    local.get $1
    local.get $0
    i32.load offset=4
    i32.load offset=12
    call_indirect $13 (type $2)
    )
  
  (func $core::slice::memchr::memchr::h9a5a49faa1649096 (type $7)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (param $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    (local $7 i32)
    (local $8 i32)
    (local $9 i32)
    (local $10 i32)
    i32.const 0
    local.set $4
    block $block
      block $block_0
        local.get $2
        i32.const 3
        i32.and
        local.tee $5
        i32.eqz
        br_if $block_0
        i32.const 4
        local.get $5
        i32.sub
        local.tee $5
        i32.eqz
        br_if $block_0
        local.get $2
        local.get $3
        local.get $5
        local.get $5
        local.get $3
        i32.gt_u
        select
        local.tee $4
        i32.add
        local.set $6
        i32.const 0
        local.set $5
        local.get $1
        i32.const 255
        i32.and
        local.set $7
        local.get $4
        local.set $8
        local.get $2
        local.set $9
        block $block_1
          loop $loop
            block $block_2
              local.get $6
              local.get $9
              i32.sub
              i32.const 3
              i32.gt_u
              br_if $block_2
              i32.const 0
              local.set $7
              local.get $1
              i32.const 255
              i32.and
              local.set $6
              loop $loop_0
                local.get $8
                i32.eqz
                br_if $block_0
                local.get $9
                local.get $7
                i32.add
                local.set $10
                local.get $8
                i32.const -1
                i32.add
                local.set $8
                local.get $7
                i32.const 1
                i32.add
                local.set $7
                local.get $10
                i32.load8_u
                local.tee $10
                local.get $6
                i32.ne
                br_if $loop_0
              end ;; $loop_0
              local.get $5
              local.get $10
              local.get $1
              i32.const 255
              i32.and
              i32.eq
              i32.const 1
              i32.add
              i32.const 1
              i32.and
              i32.add
              local.get $7
              i32.add
              i32.const -1
              i32.add
              local.set $5
              br $block_1
            end ;; $block_2
            local.get $5
            local.get $9
            i32.load8_u
            local.tee $10
            local.get $7
            i32.ne
            i32.add
            local.set $5
            local.get $10
            local.get $7
            i32.eq
            br_if $block_1
            local.get $5
            local.get $9
            i32.const 1
            i32.add
            i32.load8_u
            local.tee $10
            local.get $7
            i32.ne
            i32.add
            local.set $5
            local.get $10
            local.get $7
            i32.eq
            br_if $block_1
            local.get $5
            local.get $9
            i32.const 2
            i32.add
            i32.load8_u
            local.tee $10
            local.get $7
            i32.ne
            i32.add
            local.set $5
            local.get $10
            local.get $7
            i32.eq
            br_if $block_1
            local.get $5
            local.get $9
            i32.const 3
            i32.add
            i32.load8_u
            local.tee $10
            local.get $7
            i32.ne
            i32.add
            local.set $5
            local.get $8
            i32.const -4
            i32.add
            local.set $8
            local.get $9
            i32.const 4
            i32.add
            local.set $9
            local.get $10
            local.get $7
            i32.ne
            br_if $loop
          end ;; $loop
        end ;; $block_1
        i32.const 1
        local.set $9
        br $block
      end ;; $block_0
      local.get $1
      i32.const 255
      i32.and
      local.set $7
      block $block_3
        block $block_4
          local.get $3
          i32.const 8
          i32.lt_u
          br_if $block_4
          local.get $4
          local.get $3
          i32.const -8
          i32.add
          local.tee $10
          i32.gt_u
          br_if $block_4
          local.get $7
          i32.const 16843009
          i32.mul
          local.set $5
          block $block_5
            loop $loop_1
              local.get $2
              local.get $4
              i32.add
              local.tee $9
              i32.const 4
              i32.add
              i32.load
              local.get $5
              i32.xor
              local.tee $8
              i32.const -1
              i32.xor
              local.get $8
              i32.const -16843009
              i32.add
              i32.and
              local.get $9
              i32.load
              local.get $5
              i32.xor
              local.tee $9
              i32.const -1
              i32.xor
              local.get $9
              i32.const -16843009
              i32.add
              i32.and
              i32.or
              i32.const -2139062144
              i32.and
              br_if $block_5
              local.get $4
              i32.const 8
              i32.add
              local.tee $4
              local.get $10
              i32.le_u
              br_if $loop_1
            end ;; $loop_1
          end ;; $block_5
          local.get $4
          local.get $3
          i32.gt_u
          br_if $block_3
        end ;; $block_4
        local.get $2
        local.get $4
        i32.add
        local.set $9
        local.get $2
        local.get $3
        i32.add
        local.set $2
        local.get $3
        local.get $4
        i32.sub
        local.set $8
        i32.const 0
        local.set $5
        block $block_6
          block $block_7
            loop $loop_2
              block $block_8
                local.get $2
                local.get $9
                i32.sub
                i32.const 3
                i32.gt_u
                br_if $block_8
                i32.const 0
                local.set $7
                local.get $1
                i32.const 255
                i32.and
                local.set $2
                loop $loop_3
                  local.get $8
                  i32.eqz
                  br_if $block_6
                  local.get $9
                  local.get $7
                  i32.add
                  local.set $10
                  local.get $8
                  i32.const -1
                  i32.add
                  local.set $8
                  local.get $7
                  i32.const 1
                  i32.add
                  local.set $7
                  local.get $10
                  i32.load8_u
                  local.tee $10
                  local.get $2
                  i32.ne
                  br_if $loop_3
                end ;; $loop_3
                local.get $10
                local.get $1
                i32.const 255
                i32.and
                i32.eq
                i32.const 1
                i32.add
                i32.const 1
                i32.and
                local.get $5
                i32.add
                local.get $7
                i32.add
                i32.const -1
                i32.add
                local.set $5
                br $block_7
              end ;; $block_8
              local.get $5
              local.get $9
              i32.load8_u
              local.tee $10
              local.get $7
              i32.ne
              i32.add
              local.set $5
              local.get $10
              local.get $7
              i32.eq
              br_if $block_7
              local.get $5
              local.get $9
              i32.const 1
              i32.add
              i32.load8_u
              local.tee $10
              local.get $7
              i32.ne
              i32.add
              local.set $5
              local.get $10
              local.get $7
              i32.eq
              br_if $block_7
              local.get $5
              local.get $9
              i32.const 2
              i32.add
              i32.load8_u
              local.tee $10
              local.get $7
              i32.ne
              i32.add
              local.set $5
              local.get $10
              local.get $7
              i32.eq
              br_if $block_7
              local.get $5
              local.get $9
              i32.const 3
              i32.add
              i32.load8_u
              local.tee $10
              local.get $7
              i32.ne
              i32.add
              local.set $5
              local.get $8
              i32.const -4
              i32.add
              local.set $8
              local.get $9
              i32.const 4
              i32.add
              local.set $9
              local.get $10
              local.get $7
              i32.ne
              br_if $loop_2
            end ;; $loop_2
          end ;; $block_7
          i32.const 1
          local.set $9
          local.get $5
          local.get $4
          i32.add
          local.set $5
          br $block
        end ;; $block_6
        i32.const 0
        local.set $9
        local.get $5
        local.get $7
        i32.add
        local.get $4
        i32.add
        local.set $5
        br $block
      end ;; $block_3
      local.get $4
      local.get $3
      call $core::slice::slice_index_order_fail::h52dc23ecec067d2f
      unreachable
    end ;; $block
    local.get $0
    local.get $5
    i32.store offset=4
    local.get $0
    local.get $9
    i32.store
    )
  
  (func $core::ptr::real_drop_in_place::h4acead9551505031 (type $5)
    (param $0 i32)
    )
  
  (func $core::fmt::Write::write_char::hddeb2d341a4f7c8e (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    global.get $15
    i32.const 16
    i32.sub
    local.tee $2
    global.set $15
    local.get $2
    i32.const 0
    i32.store offset=12
    block $block
      block $block_0
        block $block_1
          local.get $1
          i32.const 128
          i32.lt_u
          br_if $block_1
          local.get $1
          i32.const 2048
          i32.lt_u
          br_if $block_0
          block $block_2
            local.get $1
            i32.const 65536
            i32.ge_u
            br_if $block_2
            local.get $2
            local.get $1
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=14
            local.get $2
            local.get $1
            i32.const 6
            i32.shr_u
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=13
            local.get $2
            local.get $1
            i32.const 12
            i32.shr_u
            i32.const 15
            i32.and
            i32.const 224
            i32.or
            i32.store8 offset=12
            i32.const 3
            local.set $1
            br $block
          end ;; $block_2
          local.get $2
          local.get $1
          i32.const 63
          i32.and
          i32.const 128
          i32.or
          i32.store8 offset=15
          local.get $2
          local.get $1
          i32.const 18
          i32.shr_u
          i32.const 240
          i32.or
          i32.store8 offset=12
          local.get $2
          local.get $1
          i32.const 6
          i32.shr_u
          i32.const 63
          i32.and
          i32.const 128
          i32.or
          i32.store8 offset=14
          local.get $2
          local.get $1
          i32.const 12
          i32.shr_u
          i32.const 63
          i32.and
          i32.const 128
          i32.or
          i32.store8 offset=13
          i32.const 4
          local.set $1
          br $block
        end ;; $block_1
        local.get $2
        local.get $1
        i32.store8 offset=12
        i32.const 1
        local.set $1
        br $block
      end ;; $block_0
      local.get $2
      local.get $1
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=13
      local.get $2
      local.get $1
      i32.const 6
      i32.shr_u
      i32.const 31
      i32.and
      i32.const 192
      i32.or
      i32.store8 offset=12
      i32.const 2
      local.set $1
    end ;; $block
    local.get $0
    local.get $2
    i32.const 12
    i32.add
    local.get $1
    call $<core::fmt::builders::PadAdapter_as_core::fmt::Write>::write_str::h3cfa2330727f68d0
    local.set $1
    local.get $2
    i32.const 16
    i32.add
    global.set $15
    local.get $1
    )
  
  (func $core::fmt::Write::write_fmt::hdcf4af1ee4f15333 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    global.get $15
    i32.const 32
    i32.sub
    local.tee $2
    global.set $15
    local.get $2
    local.get $0
    i32.store offset=4
    local.get $2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get $1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $2
    local.get $1
    i64.load align=4
    i64.store offset=8
    local.get $2
    i32.const 4
    i32.add
    i32.const 1054640
    local.get $2
    i32.const 8
    i32.add
    call $core::fmt::write::h9ffb25a5a03fc281
    local.set $1
    local.get $2
    i32.const 32
    i32.add
    global.set $15
    local.get $1
    )
  
  (func $<&mut_W_as_core::fmt::Write>::write_str::heb9faa6c9b7a53d6 (type $1)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    local.get $0
    i32.load
    local.get $1
    local.get $2
    call $<core::fmt::builders::PadAdapter_as_core::fmt::Write>::write_str::h3cfa2330727f68d0
    )
  
  (func $<&mut_W_as_core::fmt::Write>::write_char::h66c6e1685ede49cb (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    global.get $15
    i32.const 16
    i32.sub
    local.tee $2
    global.set $15
    local.get $0
    i32.load
    local.set $0
    local.get $2
    i32.const 0
    i32.store offset=12
    block $block
      block $block_0
        block $block_1
          local.get $1
          i32.const 128
          i32.lt_u
          br_if $block_1
          local.get $1
          i32.const 2048
          i32.lt_u
          br_if $block_0
          block $block_2
            local.get $1
            i32.const 65536
            i32.ge_u
            br_if $block_2
            local.get $2
            local.get $1
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=14
            local.get $2
            local.get $1
            i32.const 6
            i32.shr_u
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=13
            local.get $2
            local.get $1
            i32.const 12
            i32.shr_u
            i32.const 15
            i32.and
            i32.const 224
            i32.or
            i32.store8 offset=12
            i32.const 3
            local.set $1
            br $block
          end ;; $block_2
          local.get $2
          local.get $1
          i32.const 63
          i32.and
          i32.const 128
          i32.or
          i32.store8 offset=15
          local.get $2
          local.get $1
          i32.const 18
          i32.shr_u
          i32.const 240
          i32.or
          i32.store8 offset=12
          local.get $2
          local.get $1
          i32.const 6
          i32.shr_u
          i32.const 63
          i32.and
          i32.const 128
          i32.or
          i32.store8 offset=14
          local.get $2
          local.get $1
          i32.const 12
          i32.shr_u
          i32.const 63
          i32.and
          i32.const 128
          i32.or
          i32.store8 offset=13
          i32.const 4
          local.set $1
          br $block
        end ;; $block_1
        local.get $2
        local.get $1
        i32.store8 offset=12
        i32.const 1
        local.set $1
        br $block
      end ;; $block_0
      local.get $2
      local.get $1
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=13
      local.get $2
      local.get $1
      i32.const 6
      i32.shr_u
      i32.const 31
      i32.and
      i32.const 192
      i32.or
      i32.store8 offset=12
      i32.const 2
      local.set $1
    end ;; $block
    local.get $0
    local.get $2
    i32.const 12
    i32.add
    local.get $1
    call $<core::fmt::builders::PadAdapter_as_core::fmt::Write>::write_str::h3cfa2330727f68d0
    local.set $1
    local.get $2
    i32.const 16
    i32.add
    global.set $15
    local.get $1
    )
  
  (func $<&mut_W_as_core::fmt::Write>::write_fmt::hf64f7e3f726250ce (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    global.get $15
    i32.const 32
    i32.sub
    local.tee $2
    global.set $15
    local.get $2
    local.get $0
    i32.load
    i32.store offset=4
    local.get $2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get $1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $2
    local.get $1
    i64.load align=4
    i64.store offset=8
    local.get $2
    i32.const 4
    i32.add
    i32.const 1054640
    local.get $2
    i32.const 8
    i32.add
    call $core::fmt::write::h9ffb25a5a03fc281
    local.set $1
    local.get $2
    i32.const 32
    i32.add
    global.set $15
    local.get $1
    )
  
  (func $<core::fmt::Error_as_core::fmt::Debug>::fmt::h99349d0cc7fe11e2 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    local.get $1
    i32.load offset=24
    i32.const 1054671
    i32.const 5
    local.get $1
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect $13 (type $1)
    )
  
  (func $memcpy (type $1)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    (local $3 i32)
    block $block
      local.get $2
      i32.eqz
      br_if $block
      local.get $0
      local.set $3
      loop $loop
        local.get $3
        local.get $1
        i32.load8_u
        i32.store8
        local.get $3
        i32.const 1
        i32.add
        local.set $3
        local.get $1
        i32.const 1
        i32.add
        local.set $1
        local.get $2
        i32.const -1
        i32.add
        local.tee $2
        br_if $loop
      end ;; $loop
    end ;; $block
    local.get $0
    )
  
  (func $memcmp (type $1)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    i32.const 0
    local.set $3
    block $block
      local.get $2
      i32.eqz
      br_if $block
      block $block_0
        loop $loop
          local.get $0
          i32.load8_u
          local.tee $4
          local.get $1
          i32.load8_u
          local.tee $5
          i32.ne
          br_if $block_0
          local.get $1
          i32.const 1
          i32.add
          local.set $1
          local.get $0
          i32.const 1
          i32.add
          local.set $0
          local.get $2
          i32.const -1
          i32.add
          local.tee $2
          i32.eqz
          br_if $block
          br $loop
        end ;; $loop
      end ;; $block_0
      local.get $4
      local.get $5
      i32.sub
      local.set $3
    end ;; $block
    local.get $3
    )
  ;; User section "producers":
    ;; "\02\08language\01\04Rust\00\0cprocessed-by\01\05"
    ;; "rustc%1.39.0-nightly (2111aed0a "
    ;; "2019-08-17)"
  )