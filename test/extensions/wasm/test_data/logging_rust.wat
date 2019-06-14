(module
  (type $0 (func (param i32 i32)))
  (type $1 (func (param i32 i32 i32) (result i32)))
  (type $2 (func (param i32 i32) (result i32)))
  (type $3 (func (param i32 i32 i32)))
  (type $4 (func))
  (type $5 (func (param i32)))
  (type $6 (func (param i32) (result i32)))
  (type $7 (func (param i32 i32 i32 i32 i32) (result i32)))
  (type $8 (func (param i32) (result i64)))
  (type $9 (func (param i32 i32 i32 i32)))
  (import "env" "_proxy_log" (func $_proxy_log (param i32 i32 i32)))
  (export "memory" (memory $11))
  (export "__indirect_function_table" (table $10))
  (export "__heap_base" (global $13))
  (export "__data_end" (global $14))
  (export "_proxy_onConfigure" (func $_proxy_onConfigure))
  (export "_proxy_onStart" (func $_proxy_onStart))
  (export "_proxy_onTick" (func $_proxy_onTick))
  (export "__post_instantiate" (func $__post_instantiate))
  (export "_malloc" (func $_malloc))
  (export "_free" (func $_free))
  (memory $11  17)
  (table $10  28 28 funcref)
  (global $12  (mut i32) (i32.const 1048576))
  (global $13  i32 (i32.const 1055604))
  (global $14  i32 (i32.const 1055604))
  (elem $10 (i32.const 1)
    $<&T_as_core::fmt::Display>::fmt::h05feff42ddc07882 $<core::fmt::Error_as_core::fmt::Debug>::fmt::hb56878928eceb98a $<log::SetLoggerError_as_core::fmt::Debug>::fmt::ha942b054345bd721 $<&T_as_core::fmt::Display>::fmt::h3f145b24ed245033 $core::fmt::ArgumentV1::show_usize::h7e76c008016dd45f $core::fmt::num::<impl_core::fmt::Display_for_u32>::fmt::h4f4d56a15c6233ed $<core::ops::range::Range<Idx>_as_core::fmt::Debug>::fmt::hd43772a3c2cd5a20 $<char_as_core::fmt::Debug>::fmt::hda62120a64941310
    $core::fmt::num::<impl_core::fmt::Debug_for_usize>::fmt::h2a5aa117bff2c084 $core::ptr::real_drop_in_place::h181e24faf7b583a9 $<proxy_wasm::Logger_as_log::Log>::enabled::hdf38864a8a45225e $<proxy_wasm::Logger_as_log::Log>::log::h2c2b3343c1e0084a $<log::NopLogger_as_log::Log>::flush::hd7e64e1b27d09f04 $core::ptr::real_drop_in_place::h0e270dec7857294a $<&mut_W_as_core::fmt::Write>::write_str::hccd6f22edec2e385 $<&mut_W_as_core::fmt::Write>::write_char::hc45ff4c6ed2a2042
    $<&mut_W_as_core::fmt::Write>::write_fmt::hc3368130906df163 $<log::NopLogger_as_log::Log>::enabled::h1a8020a0a9c8ab7b $<log::NopLogger_as_log::Log>::log::h3310d0bba2c563ff $<T_as_core::any::Any>::get_type_id::h84858aded9b6f3c7 $core::ptr::real_drop_in_place::h7a6e9b3a26682e2c $<core::fmt::builders::PadAdapter<'_>_as_core::fmt::Write>::write_str::heb6658a92b0d254f $core::fmt::Write::write_char::h699f698034020179 $core::fmt::Write::write_fmt::h88700fc7b71e07ba
    $<&mut_W_as_core::fmt::Write>::write_str::h5e939bfff6b7262a $<&mut_W_as_core::fmt::Write>::write_char::h6c366a2088812c15 $<&mut_W_as_core::fmt::Write>::write_fmt::h1bb28d0d3f773a6a)
  (data $11 (i32.const 1048576)
    "warn logging_rustsrc/lib.rstest trace loggingtest debug loggingt"
    "est error loggingtest tick loggingsrc/libcore/result.rs: \00\00\00\01\00\00\00"
    "\00\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00a Display implementation returne"
    "d an error unexpectedlycalled `Result::unwrap()` on an `Err` val"
    "ueTried to shrink to a larger capacity\00\00SetLoggerError()capacity"
    " overflowsrc/liballoc/raw_vec.rs\01\00\00\00\00\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00"
    "\03\00\00\00\01\00\00\00\01\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00index out of bounds: the"
    " len is  but the index is 00010203040506070809101112131415161718"
    "1920212223242526272829303132333435363738394041424344454647484950"
    "5152535455565758596061626364656667686970717273747576777879808182"
    "8384858687888990919293949596979899src/libcore/slice/mod.rsindex "
    " out of range for slice of length slice index starts at  but end"
    "s at `0x[...]\00\00\00\01\00\00\00\00\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\01\00\00\00 \00\00\00"
    "\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\02\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00"
    "\00\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\01\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00"
    "\03\00\00\00\01\00\00\00\02\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\03\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00"
    "\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\00\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\01\00\00\00 \00\00\00\00\00\00\00"
    "\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\02\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\03\00\00\00"
    " \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\04\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00"
    "src/libcore/str/mod.rsbyte index  is not a char boundary; it is "
    "inside  (bytes ) of `\00\00\00\00\00\00\00\00\00\00\00src/libcore/fmt/mod.rs..\00\01\03\05\05\06\06\03"
    "\07\06\08\08\09\11\n\1c\0b\19\0c\14\0d\12\0e\16\0f\04\10\03\12\12\13\09\16\01\17\05\18\02\19\03\1a\07\1c\02\1d\01\1f\16 \03+\06,\02-\0b.\010\031\022\02\a9\02\aa\04\ab\08\fa\02"
    "\fb\05\fd\04\fe\03\ff\09\adxy\8b\8d\a20WX\8b\8c\90\1c\1d\dd\0e\0fKL\fb\fc./?\\]_\b5\e2\84\8d\8e\91\92\a9\b1\ba\bb\c5\c6\c9\ca\de\e4\e5\ff\00\04\11\12)147:;"
    "=IJ]\84\8e\92\a9\b1\b4\ba\bb\c6\ca\ce\cf\e4\e5\00\04\0d\0e\11\12)14:;EFIJ^de\84\91\9b\9d\c9\ce\cf\0d\11)EIWde\8d\91\a9\b4\ba\bb\c5\c9\df\e4\e5\f0\04"
    "\0d\11EIde\80\81\84\b2\bc\be\bf\d5\d7\f0\f1\83\85\86\89\8b\8c\98\a0\a4\a6\a8\a9\ac\ba\be\bf\c5\c7\ce\cf\da\dbH\98\bd\cd\c6\ce\cfINOWY^_\89\8e\8f\b1\b6\b7\bf\c1\c6\c7\d7"
    "\11\16\17[\\\f6\f7\fe\ff\80\0dmq\de\df\0e\0f\1fno\1c\1d_}~\ae\af\bb\bc\fa\16\17\1e\1fFGNOXZ\\^~\7f\b5\c5\d4\d5\dc\f0\f1\f5rs\8ftu\96\97\c9\ff/_&"
    "./\a7\af\b7\bf\c7\cf\d7\df\9a@\97\980\8f\1f\ff\ce\ffNOZ[\07\08\0f\10'/\ee\efno7=?BE\90\91\fe\ffSgu\c8\c9\d0\d1\d8\d9\e7\fe\ff\00 _\"\82\df\04\82D"
    "\08\1b\04\06\11\81\ac\0e\80\ab5\1e\15\80\e0\03\19\08\01\04/\044\04\07\03\01\07\06\07\11\nP\0f\12\07U\08\02\04\1c\n\09\03\08\03\07\03\02\03\03\03\0c\04\05\03\0b\06\01\0e\15\05:\03"
    "\11\07\06\05\10\08V\07\02\07\15\0dP\04C\03-\03\01\04\11\06\0f\0c:\04\1d%\0d\06L m\04j%\80\c8\05\82\b0\03\1a\06\82\fd\03Y\07\15\0b\17\09\14\0c\14\0cj\06\n\06\1a\06Y"
    "\07+\05F\n,\04\0c\04\01\031\0b,\04\1a\06\0b\03\80\ac\06\n\06\1fAL\04-\03t\08<\03\0f\03<\078\08*\06\82\ff\11\18\08/\11-\03 \10!\0f\80\8c\04\82\97\19\0b\15\88"
    "\94\05/\05;\07\02\0e\18\09\80\af1t\0c\80\d6\1a\0c\05\80\ff\05\80\b6\05$\0c\9b\c6\n\d20\10\84\8d\037\09\81\\\14\80\b8\08\80\ba=5\04\n\068\08F\08\0c\06t\0b\1e\03Z\04"
    "Y\09\80\83\18\1c\n\16\09F\n\80\8a\06\ab\a4\0c\17\041\a1\04\81\da&\07\0c\05\05\80\a5\11\81m\10x(*\06L\04\80\8d\04\80\be\03\1b\03\0f\0d\00\06\01\01\03\01\04\02\08\08\09\02\n"
    "\05\0b\02\10\01\11\04\12\05\13\11\14\02\15\02\17\02\1a\02\1c\05\1d\08$\01j\03k\02\bc\02\d1\02\d4\0c\d5\09\d6\02\d7\02\da\01\e0\05\e8\02\ee \f0\04\f9\04\0c';>NO\8f\9e\9e\9f\06"
    "\07\096=>V\f3\d0\d1\04\14\1867VW\bd5\ce\cf\e0\12\87\89\8e\9e\04\0d\0e\11\12)14:EFIJNOdeZ\\\b6\b7\1b\1c\84\85\097\90\91\a8\07\n;>fi\8f\92"
    "o_\ee\efZb\9a\9b'(U\9d\a0\a1\a3\a4\a7\a8\ad\ba\bc\c4\06\0b\0c\15\1d:?EQ\a6\a7\cc\cd\a0\07\19\1a\"%\c5\c6\04 #%&(38:HJLPSUVXZ\\^`"
    "cefksx}\7f\8a\a4\aa\af\b0\c0\d0?qr{^\"{\05\03\04-\03e\04\01/.\80\82\1d\031\0f\1c\04$\09\1e\05+\05D\04\0e*\80\aa\06$\04$\04(\084\0b\01\80\90"
    "\817\09\16\n\08\80\989\03c\08\090\16\05!\03\1b\05\01@8\04K\05/\04\n\07\09\07@ '\04\0c\096\03:\05\1a\07\04\0c\07PI73\0d3\07.\08\n\81&\1f\80\81(\08"
    "*\80\a6N\04\1e\0fC\0e\19\07\n\06G\09'\09u\0b?A*\06;\05\n\06Q\06\01\05\10\03\05\80\8b_!H\08\n\80\a6^\"E\0b\n\06\0d\138\08\n6,\04\10\80\c0<dS\0c"
    "\01\81\00H\08S\1d9\81\07F\n\1d\03GI7\03\0e\08\n\069\07\n\816\19\81\07\83\9afu\0b\80\c4\8a\bc\84/\8f\d1\82G\a1\b9\829\07*\04\02`&\nF\n(\05\13\82\b0["
    "eE\0b/\10\11@\02\1e\97\f2\0e\82\f3\a5\0d\81\1fQ\81\8c\89\04k\05\0d\03\09\07\10\93`\80\f6\ns\08n\17F\80\9a\14\0cW\09\19\80\87\81G\03\85B\0f\15\85P+\87\d5\80\d7)"
    "K\05\n\04\02\83\11D\81K<\06\01\04U\05\1b4\02\81\0e,\04d\0cV\n\0d\03\\\04=9\1d\0d,\04\09\07\02\0e\06\80\9a\83\d5\0b\0d\03\n\06t\0cY'\0c\048\08\n\06(\08\1e"
    "R\0c\04g\03)\0d\n\06\03\0d0`\0e\85\92src/libcore/unicode/bool_trie.rs\00\00\c0\fb\ef>\00\00\00\00\00\0e\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\f8\ff\fb\ff\ff\ff\07\00\00\00\00\00\00\14\fe!\fe\00\0c\00\00\00\02\00\00\00\00\00\00P\1e \80\00\0c\00\00@\06\00\00\00\00\00\00\10\869\02\00\00\00#\00"
    "\be!\00\00\0c\00\00\fc\02\00\00\00\00\00\00\d0\1e \c0\00\0c\00\00\00\04\00\00\00\00\00\00@\01 \80\00\00\00\00\00\11\00\00\00\00\00\00\c0\c1=`\00\0c\00\00\00\02\00\00\00\00\00\00\90"
    "D0`\00\0c\00\00\00\03\00\00\00\00\00\00X\1e \80\00\0c\00\00\00\00\84\\\80\00\00\00\00\00\00\00\00\00\00\f2\07\80\7f\00\00\00\00\00\00\00\00\00\00\00\00\f2\1b\00?\00\00\00\00\00\00"
    "\00\00\00\03\00\00\a0\02\00\00\00\00\00\00\fe\7f\df\e0\ff\fe\ff\ff\ff\1f@\00\00\00\00\00\00\00\00\00\00\00\00\e0\fdf\00\00\00\c3\01\00\1e\00d \00 \00\00\00\00\00\00\00\e0\00\00\00\00"
    "\00\00\1c\00\00\00\1c\00\00\00\0c\00\00\00\0c\00\00\00\00\00\00\00\b0?@\fe\0f \00\00\00\00\008\00\00\00\00\00\00`\00\00\00\00\02\00\00\00\00\00\00\87\01\04\0e\00\00\80\09\00\00\00\00"
    "\00\00@\7f\e5\1f\f8\9f\00\00\00\00\00\00\ff\7f\0f\00\00\00\00\00\d0\17\04\00\00\00\00\f8\0f\00\03\00\00\00<;\00\00\00\00\00\00@\a3\03\00\00\00\00\00\00\f0\cf\00\00\00\f7\ff\fd!\10\03"
    "\ff\ff\ff\ff\ff\ff\ff\fb\00\10\00\00\00\00\00\00\00\00\ff\ff\ff\ff\01\00\00\00\00\00\00\80\03\00\00\00\00\00\00\00\00\80\00\00\00\00\ff\ff\ff\ff\00\00\00\00\00\fc\00\00\00\00\00\06\00\00\00\00"
    "\00\00\00\00\00\80\f7?\00\00\00\c0\00\00\00\00\00\00\00\00\00\00\03\00D\08\00\00`\00\00\000\00\00\00\ff\ff\03\80\00\00\00\00\c0?\00\00\80\ff\03\00\00\00\00\00\07\00\00\00\00\00\c8\13"
    "\00\00\00\00 \00\00\00\00\00\00\00\00~f\00\08\10\00\00\00\00\00\10\00\00\00\00\00\00\9d\c1\02\00\00\00\000@\00\00\00\00\00 !\00\00\00\00\00@\00\00\00\00\ff\ff\00\00\ff\ff\00\00"
    "\00\00\00\00\00\00\00\01\00\00\00\02\00\03\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\04\00\00\05\00\00\00\00\00\00\00\00\06\00\00\00\00\00\00\00\00\07\00\00"
    "\08\09\n\00\0b\0c\0d\0e\0f\00\00\10\11\12\00\00\13\14\15\16\00\00\17\18\19\1a\1b\00\1c\00\00\00\1d\00\00\00\00\00\00\00\1e\1f \00\00\00\00\00!\00\"\00#$%\00\00\00\00&\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00'(\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00)\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00*\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00+,\00\00-\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00./0\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "1\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\002\003\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "45\00\005556\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00 \00\00\00\00\01\00\00\00\00\00\00\00\00\00\c0\07n\f0\00\00\00\00\00\87\00\00\00\00`\00\00\00\00\00\00\00\f0\00\00\00\c0\ff\01\00\00\00\00\00"
    "\02\00\00\00\00\00\00\ff\7f\00\00\00\00\00\00\80\03\00\00\00\00\00x\06\07\00\00\00\80\ef\1f\00\00\00\00\00\00\00\08\00\03\00\00\00\00\00\c0\7f\00\1e\00\00\00\00\00\00\00\00\00\00\00\80\d3@"
    "\00\00\00\80\f8\07\00\00\03\00\00\00\00\00\00X\01\00\80\00\c0\1f\1f\00\00\00\00\00\00\00\00\ff\\\00\00@\00\00\00\00\00\00\00\00\00\00\f9\a5\0d\00\00\00\00\00\00\00\00\00\00\00\00\80<\b0"
    "\01\00\000\00\00\00\00\00\00\00\00\00\00\f8\a7\01\00\00\00\00\00\00\00\00\00\00\00\00(\bf\00\00\00\00\e0\bc\0f\00\00\00\00\00\00\00\80\ff\06\fe\07\00\00\00\00\f8y\80\00~\0e\00\00\00\00"
    "\00\fc\7f\03\00\00\00\00\00\00\00\00\00\00\7f\bf\00\00\fc\ff\ff\fcm\00\00\00\00\00\00\00~\b4\bf\00\00\00\00\00\00\00\00\00\a3\00\00\00\00\00\00\00\00\00\00\00\18\00\00\00\00\00\00\00\1f\00"
    "\00\00\00\00\00\00\7f\00\00\80\07\00\00\00\00\00\00\00\00`\00\00\00\00\00\00\00\00\a0\c3\07\f8\e7\0f\00\00\00<\00\00\1c\00\00\00\00\00\00\00\ff\ff\ff\ff\ff\ff\7f\f8\ff\ff\ff\ff\ff\1f \00"
    "\10\00\00\f8\fe\ff\00\00\7f\ff\ff\f9\db\07\00\00\00\00\7f\00\00\00\00\00\f0\07\00\00\00\00\00\00\00\00\00\00\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\00\00called `"
    "Option::unwrap()` on a `None` valuesrc/libcore/option.rsbegin <="
    " end ( <= ) when slicing ` is out of bounds of `    \n()Error")
  (data $11 (i32.const 1052928)
    "\00\00\10\00\05\00\00\00\1b\00\10\00\12\00\00\00-\00\10\00\12\00\00\00?\00\10\00\12\00\00\00Q\00\10\00\11\00\00\00\a8\01\10\00\00\00\00\00w\00\10\00\02\00\00\00b\00\10\00\15\00\00\00"
    "\e5\03\00\00\05\00\00\00\n\00\00\00\00\00\00\00\01\00\00\00\0b\00\00\00\0c\00\00\00\0d\00\00\00\a8\01\10\00\00\00\00\00\02\01\10\00$\00\00\00I\01\10\00\17\00\00\00A\02\00\00\09\00\00\00"
    "\0e\00\00\00\04\00\00\00\04\00\00\00\0f\00\00\00\10\00\00\00\11\00\00\00\a0\19\10\00\a0\11\10\00\n\00\00\00\00\00\00\00\01\00\00\00\12\00\00\00\13\00\00\00\0d\00\00\00\n\00\00\00\00\00\00\00"
    "\01\00\00\00\12\00\00\00\13\00\00\00\0d\00\00\008\01\10\00\11\00\00\00I\01\10\00\17\00\00\00\eb\02\00\00\05\00\00\00\a8\01\10\00 \00\00\00\c8\01\10\00\12\00\00\00\n\00\00\00\00\00\00\00"
    "\01\00\00\00\14\00\00\00\ba\02\10\00\06\00\00\00\c0\02\10\00\"\00\00\00\a2\02\10\00\18\00\00\00%\09\00\00\05\00\00\00\e2\02\10\00\16\00\00\00\f8\02\10\00\0d\00\00\00\a2\02\10\00\18\00\00\00"
    "+\09\00\00\05\00\00\00\d6\04\10\00\0b\00\00\00\da\10\10\00\16\00\00\00\05\03\10\00\01\00\00\00\c0\04\10\00\16\00\00\00%\08\00\00\09\00\00\00\b8\10\10\00\0e\00\00\00\c6\10\10\00\04\00\00\00"
    "\ca\10\10\00\10\00\00\00\05\03\10\00\01\00\00\00\c0\04\10\00\16\00\00\00)\08\00\00\05\00\00\00x\10\10\00+\00\00\00\a3\10\10\00\15\00\00\00Y\01\00\00\15\00\00\00\d6\04\10\00\0b\00\00\00"
    "\e1\04\10\00&\00\00\00\07\05\10\00\08\00\00\00\0f\05\10\00\06\00\00\00\05\03\10\00\01\00\00\00\c0\04\10\00\16\00\00\006\08\00\00\05\00\00\00\a8\01\10\00\00\00\00\006\05\10\00\02\00\00\00"
    " \05\10\00\16\00\00\004\04\00\00\11\00\00\00 \05\10\00\16\00\00\00(\04\00\00(\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\ff\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\f8\03\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\fe\ff\ff\ff\ff\bf\b6\00\00\00\00\00\00\00\00\00\ff\07\00\00\00\00\00\f8\ff\ff\00\00\01\00\00\00\00\00\00\00\00\00\00\00\c0\9f\9f=\00\00"
    "\00\00\02\00\00\00\ff\ff\ff\07\00\00\00\00\00\00\00\00\00\00\c0\ff\01\00\00\00\00\00\00\f8\0f p\n\10\00J\00\00\00\c0\0c\10\00\00\02\00\00\c0\0e\10\007\00\00\00\00\01\02\03\04\05\06\07"
    "\08\09\08\n\0b\0c\0d\0e\0f\10\11\12\13\14\02\15\16\17\18\19\1a\1b\1c\1d\1e\1f \02\02\02\02\02\02\02\02\02\02!\02\02\02\02\02\02\02\02\02\02\02\02\02\02\"#$%&\02'\02(\02\02\02"
    ")*+\02,-./0\02\021\02\02\022\02\02\02\02\02\02\02\023\02\024\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\025\026\027\02\02\02\02\02\02\02\028\029\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02:;<\02\02\02\02=\02\02>?@AB"
    "CDEF\02\02\02G\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02H\02\02\02\02\02\02\02\02\02\02\02I\02\02\02\02\02;\02\00\01\02\02\02\02\03\02\02\02\02\04\02\05\06\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\07\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02"
    "\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02P\n\10\00 \00\00\00'\00\00\00\19\00\00\00P\n\10\00 \00\00\00(\00\00\00 \00\00\00P\n\10\00 \00\00\00"
    "*\00\00\00\19\00\00\00P\n\10\00 \00\00\00+\00\00\00\18\00\00\00P\n\10\00 \00\00\00,\00\00\00 \00\00\00\15\00\00\00\0c\00\00\00\04\00\00\00\16\00\00\00\17\00\00\00\18\00\00\00"
    "\0e\00\00\00\04\00\00\00\04\00\00\00\19\00\00\00\1a\00\00\00\1b\00\00\00")
  (data $11 (i32.const 1055128)
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00"
    "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00")
  
  (func $_proxy_onConfigure (type $3)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (local $3 i32)
    (local $4 i32)
    global.get $12
    i32.const 80
    i32.sub
    local.tee $3
    global.set $12
    local.get $3
    local.get $2
    i32.store offset=4
    local.get $3
    local.get $1
    i32.store
    block $block
      i32.const 0
      i32.load offset=1055128
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
      i32.load offset=1053084
      local.set $1
      i32.const 0
      i32.load offset=1053080
      local.set $2
      i32.const 0
      i32.load offset=1055132
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
      i32.const 40
      i32.add
      i32.const 1
      i32.store
      local.get $3
      i32.const 36
      i32.add
      i32.const 1048700
      i32.store
      local.get $3
      i32.const 32
      i32.add
      i32.const 1
      i32.store
      local.get $3
      i32.const 24
      i32.add
      i32.const 12
      i32.store
      local.get $3
      i32.const 1048593
      i32.store offset=60
      local.get $3
      i32.const 1048581
      i32.store offset=52
      local.get $3
      i32.const 44
      i32.add
      local.get $3
      i32.const 8
      i32.add
      i32.store
      local.get $3
      i32.const 1052928
      i32.store offset=28
      local.get $3
      i32.const 1048581
      i32.store offset=20
      local.get $3
      i32.const 2
      i32.store offset=16
      local.get $2
      i32.const 1048872
      local.get $4
      i32.const 2
      i32.eq
      local.tee $4
      select
      local.get $3
      i32.const 16
      i32.add
      local.get $1
      i32.const 1053112
      local.get $4
      select
      i32.load offset=16
      call_indirect $10 (type $0)
    end ;; $block
    local.get $3
    i32.const 80
    i32.add
    global.set $12
    )
  
  (func $<&T_as_core::fmt::Display>::fmt::h05feff42ddc07882 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    local.get $1
    local.get $0
    i32.load
    local.get $0
    i32.load offset=4
    call $core::fmt::Formatter::pad::hfe31e8860a6945ec
    )
  
  (func $_proxy_onStart (type $3)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    (local $7 i32)
    global.get $12
    i32.const 64
    i32.sub
    local.tee $3
    global.set $12
    block $block
      block $block_0
        block $block_1
          block $block_2
            block $block_3
              i32.const 0
              i32.load offset=1055128
              i32.const 5
              i32.ge_u
              br_if $block_3
              i32.const 4
              local.set $4
              i32.const 0
              i32.load offset=1055128
              i32.const 4
              i32.ge_u
              br_if $block_2
              br $block_1
            end ;; $block_3
            i32.const 0
            i32.load offset=1053084
            local.set $4
            i32.const 0
            i32.load offset=1053080
            local.set $5
            i32.const 0
            i32.load offset=1055132
            local.set $6
            local.get $3
            i32.const 56
            i32.add
            i32.const 16
            i32.store
            local.get $3
            i32.const 48
            i32.add
            i64.const 4294967306
            i64.store
            local.get $3
            i32.const 40
            i32.add
            i32.const 12
            i32.store
            local.get $3
            i32.const 32
            i32.add
            i32.const 0
            i32.store
            local.get $3
            i32.const 28
            i32.add
            i32.const 1049000
            i32.store
            local.get $3
            i32.const 16
            i32.add
            i64.const 1
            i64.store
            local.get $3
            i32.const 8
            i32.add
            i32.const 12
            i32.store
            local.get $3
            i32.const 1048593
            i32.store offset=44
            local.get $3
            i32.const 1048581
            i32.store offset=36
            local.get $3
            i32.const 1052936
            i32.store offset=12
            local.get $3
            i32.const 1048581
            i32.store offset=4
            local.get $3
            i32.const 5
            i32.store
            local.get $5
            i32.const 1048872
            local.get $6
            i32.const 2
            i32.eq
            local.tee $6
            select
            local.get $3
            local.get $4
            i32.const 1053112
            local.get $6
            select
            i32.load offset=16
            call_indirect $10 (type $0)
            i32.const 4
            local.set $4
            i32.const 0
            i32.load offset=1055128
            i32.const 4
            i32.lt_u
            br_if $block_1
          end ;; $block_2
          i32.const 0
          i32.load offset=1053084
          local.set $5
          i32.const 0
          i32.load offset=1053080
          local.set $6
          i32.const 0
          i32.load offset=1055132
          local.set $7
          local.get $3
          i32.const 56
          i32.add
          i32.const 17
          i32.store
          local.get $3
          i32.const 48
          i32.add
          i64.const 4294967306
          i64.store
          local.get $3
          i32.const 40
          i32.add
          i32.const 12
          i32.store
          local.get $3
          i32.const 32
          i32.add
          i32.const 0
          i32.store
          local.get $3
          i32.const 28
          i32.add
          i32.const 1049000
          i32.store
          local.get $3
          i32.const 16
          i32.add
          i64.const 1
          i64.store
          local.get $3
          i32.const 8
          i32.add
          i32.const 12
          i32.store
          local.get $3
          i32.const 1048593
          i32.store offset=44
          local.get $3
          i32.const 1048581
          i32.store offset=36
          local.get $3
          i32.const 1052944
          i32.store offset=12
          local.get $3
          i32.const 1048581
          i32.store offset=4
          local.get $3
          local.get $4
          i32.store
          local.get $6
          i32.const 1048872
          local.get $7
          i32.const 2
          i32.eq
          local.tee $4
          select
          local.get $3
          local.get $5
          i32.const 1053112
          local.get $4
          select
          i32.load offset=16
          call_indirect $10 (type $0)
          i32.const 0
          local.set $4
          i32.const 0
          i32.load offset=1055128
          br_if $block_0
          br $block
        end ;; $block_1
        i32.const 0
        local.set $4
        i32.const 0
        i32.load offset=1055128
        i32.eqz
        br_if $block
      end ;; $block_0
      local.get $4
      i32.load offset=1053084
      local.set $5
      local.get $4
      i32.load offset=1053080
      local.set $6
      local.get $4
      i32.load offset=1055132
      local.set $7
      local.get $3
      i32.const 56
      i32.add
      i32.const 18
      i32.store
      local.get $3
      i32.const 48
      i32.add
      i64.const 4294967306
      i64.store
      local.get $3
      i32.const 40
      i32.add
      i32.const 12
      i32.store
      local.get $3
      i32.const 32
      i32.add
      local.get $4
      i32.store
      local.get $3
      i32.const 28
      i32.add
      i32.const 1049000
      i32.store
      local.get $3
      i32.const 16
      i32.add
      i64.const 1
      i64.store
      local.get $3
      i32.const 8
      i32.add
      i32.const 12
      i32.store
      local.get $3
      i32.const 1048593
      i32.store offset=44
      local.get $3
      i32.const 1048581
      i32.store offset=36
      local.get $3
      i32.const 1052952
      i32.store offset=12
      local.get $3
      i32.const 1048581
      i32.store offset=4
      local.get $3
      i32.const 1
      i32.store
      local.get $6
      i32.const 1048872
      local.get $7
      i32.const 2
      i32.eq
      local.tee $4
      select
      local.get $3
      local.get $5
      i32.const 1053112
      local.get $4
      select
      i32.load offset=16
      call_indirect $10 (type $0)
    end ;; $block
    local.get $3
    i32.const 64
    i32.add
    global.set $12
    )
  
  (func $_proxy_onTick (type $5)
    (param $0 i32)
    (local $1 i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    global.get $12
    i32.const 64
    i32.sub
    local.tee $1
    global.set $12
    block $block
      i32.const 0
      i32.load offset=1055128
      i32.const 3
      i32.lt_u
      br_if $block
      i32.const 0
      i32.load offset=1053084
      local.set $2
      i32.const 0
      i32.load offset=1053080
      local.set $3
      i32.const 0
      i32.load offset=1055132
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
      i32.const 1049000
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
      i32.const 1048593
      i32.store offset=44
      local.get $1
      i32.const 1048581
      i32.store offset=36
      local.get $1
      i32.const 1052960
      i32.store offset=12
      local.get $1
      i32.const 1048581
      i32.store offset=4
      local.get $1
      i32.const 3
      i32.store
      local.get $3
      i32.const 1048872
      local.get $4
      i32.const 2
      i32.eq
      local.tee $4
      select
      local.get $1
      local.get $2
      i32.const 1053112
      local.get $4
      select
      i32.load offset=16
      call_indirect $10 (type $0)
    end ;; $block
    local.get $1
    i32.const 64
    i32.add
    global.set $12
    )
  
  (func $core::result::unwrap_failed::h1d6d220c53d3f336 (type $4)
    (local $0 i32)
    global.get $12
    i32.const 64
    i32.sub
    local.tee $0
    global.set $12
    local.get $0
    i32.const 55
    i32.store offset=12
    local.get $0
    i32.const 1048736
    i32.store offset=8
    local.get $0
    i32.const 40
    i32.add
    i32.const 12
    i32.add
    i32.const 2
    i32.store
    local.get $0
    i32.const 36
    i32.add
    i32.const 2
    i32.store
    local.get $0
    i32.const 16
    i32.add
    i32.const 12
    i32.add
    i32.const 2
    i32.store
    local.get $0
    i32.const 1
    i32.store offset=44
    local.get $0
    i32.const 1048928
    i32.store offset=24
    local.get $0
    i32.const 2
    i32.store offset=20
    local.get $0
    i32.const 1052968
    i32.store offset=16
    local.get $0
    local.get $0
    i32.const 56
    i32.add
    i32.store offset=48
    local.get $0
    local.get $0
    i32.const 8
    i32.add
    i32.store offset=40
    local.get $0
    local.get $0
    i32.const 40
    i32.add
    i32.store offset=32
    local.get $0
    i32.const 16
    i32.add
    i32.const 1052984
    call $core::panicking::panic_fmt::h74ee8034b317ceed
    unreachable
    )
  
  (func $<core::fmt::Error_as_core::fmt::Debug>::fmt::hb56878928eceb98a (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    local.get $1
    i32.load offset=24
    i32.const 1052919
    i32.const 5
    local.get $1
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect $10 (type $1)
    )
  
  (func $core::panicking::panic_fmt::h74ee8034b317ceed (type $0)
    (param $0 i32)
    (param $1 i32)
    (local $2 i32)
    (local $3 i64)
    global.get $12
    i32.const 32
    i32.sub
    local.tee $2
    global.set $12
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
    i32.const 1053176
    i32.store offset=4
    local.get $2
    i32.const 1049000
    i32.store
    local.get $2
    call $rust_begin_unwind
    unreachable
    )
  
  (func $core::result::unwrap_failed::h53c13222d410a969 (type $4)
    (local $0 i32)
    global.get $12
    i32.const 64
    i32.sub
    local.tee $0
    global.set $12
    local.get $0
    i32.const 43
    i32.store offset=12
    local.get $0
    i32.const 1048791
    i32.store offset=8
    local.get $0
    i32.const 40
    i32.add
    i32.const 12
    i32.add
    i32.const 3
    i32.store
    local.get $0
    i32.const 36
    i32.add
    i32.const 2
    i32.store
    local.get $0
    i32.const 16
    i32.add
    i32.const 12
    i32.add
    i32.const 2
    i32.store
    local.get $0
    i32.const 1
    i32.store offset=44
    local.get $0
    i32.const 1048928
    i32.store offset=24
    local.get $0
    i32.const 2
    i32.store offset=20
    local.get $0
    i32.const 1052968
    i32.store offset=16
    local.get $0
    local.get $0
    i32.const 56
    i32.add
    i32.store offset=48
    local.get $0
    local.get $0
    i32.const 8
    i32.add
    i32.store offset=40
    local.get $0
    local.get $0
    i32.const 40
    i32.add
    i32.store offset=32
    local.get $0
    i32.const 16
    i32.add
    i32.const 1052984
    call $core::panicking::panic_fmt::h74ee8034b317ceed
    unreachable
    )
  
  (func $<log::SetLoggerError_as_core::fmt::Debug>::fmt::ha942b054345bd721 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    global.get $12
    i32.const 80
    i32.sub
    local.tee $2
    global.set $12
    i32.const 1
    local.set $3
    block $block
      local.get $1
      i32.load offset=24
      i32.const 1048872
      i32.const 14
      local.get $1
      i32.const 28
      i32.add
      i32.load
      i32.load offset=12
      call_indirect $10 (type $1)
      br_if $block
      block $block_0
        block $block_1
          local.get $1
          i32.load
          local.tee $3
          i32.const 4
          i32.and
          br_if $block_1
          i32.const 1
          local.set $3
          local.get $1
          i32.const 24
          i32.add
          local.tee $4
          i32.load
          i32.const 1052917
          i32.const 1
          local.get $1
          i32.const 28
          i32.add
          local.tee $5
          i32.load
          i32.load offset=12
          call_indirect $10 (type $1)
          br_if $block
          local.get $4
          i32.load
          i32.const 1049000
          i32.const 0
          local.get $5
          i32.load
          i32.load offset=12
          call_indirect $10 (type $1)
          br_if $block
          local.get $1
          i32.const 1048886
          i32.const 2
          call $core::fmt::Formatter::pad::hfe31e8860a6945ec
          br_if $block
          br $block_0
        end ;; $block_1
        local.get $2
        i32.const 52
        i32.add
        i32.const 1055080
        i32.store
        local.get $2
        i32.const 0
        i32.store8 offset=16
        local.get $2
        local.get $3
        i32.store offset=24
        local.get $2
        local.get $1
        i32.const 24
        i32.add
        i64.load align=4
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
        i64.load offset=8 align=4
        i64.store offset=32
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
        local.get $2
        i32.const 8
        i32.add
        i32.store offset=48
        i32.const 1
        local.set $3
        local.get $2
        i32.const 8
        i32.add
        i32.const 1052917
        i32.const 1
        call $<core::fmt::builders::PadAdapter<'_>_as_core::fmt::Write>::write_str::heb6658a92b0d254f
        br_if $block
        local.get $2
        i32.const 8
        i32.add
        i32.const 1052916
        i32.const 1
        call $<core::fmt::builders::PadAdapter<'_>_as_core::fmt::Write>::write_str::heb6658a92b0d254f
        br_if $block
        i32.const 1
        local.set $3
        local.get $2
        i32.const 24
        i32.add
        i32.const 1048886
        i32.const 2
        call $core::fmt::Formatter::pad::hfe31e8860a6945ec
        br_if $block
      end ;; $block_0
      block $block_2
        local.get $1
        i32.load8_u
        i32.const 4
        i32.and
        i32.eqz
        br_if $block_2
        i32.const 1
        local.set $3
        local.get $1
        i32.const 24
        i32.add
        i32.load
        i32.const 1052916
        i32.const 1
        local.get $1
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect $10 (type $1)
        br_if $block
      end ;; $block_2
      local.get $1
      i32.const 24
      i32.add
      i32.load
      i32.const 1052918
      i32.const 1
      local.get $1
      i32.const 28
      i32.add
      i32.load
      i32.load offset=12
      call_indirect $10 (type $1)
      local.set $1
      local.get $2
      i32.const 80
      i32.add
      global.set $12
      local.get $1
      return
    end ;; $block
    local.get $2
    i32.const 80
    i32.add
    global.set $12
    local.get $3
    )
  
  (func $<proxy_wasm::Logger_as_log::Log>::enabled::hdf38864a8a45225e (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    i32.const 1
    )
  
  (func $<proxy_wasm::Logger_as_log::Log>::log::h2c2b3343c1e0084a (type $0)
    (param $0 i32)
    (param $1 i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    global.get $12
    i32.const 64
    i32.sub
    local.tee $2
    global.set $12
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
    i32.const 40
    i32.add
    i32.const 12
    i32.add
    i32.const 1
    i32.store
    local.get $2
    i32.const 1048700
    i32.store offset=48
    local.get $2
    i32.const 1
    i32.store offset=44
    local.get $2
    i32.const 1053024
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
          i32.const 1053056
          local.get $2
          i32.const 40
          i32.add
          call $core::fmt::write::h2bc7cdeb5edd0d71
          br_if $block_1
          block $block_2
            block $block_3
              local.get $2
              i32.load offset=12
              local.tee $1
              local.get $2
              i32.const 16
              i32.add
              i32.load
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
              i32.eqz
              br_if $block_4
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
              br $block_2
            end ;; $block_4
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
          end ;; $block_2
          i32.const 5
          local.get $3
          i32.sub
          local.get $5
          local.get $2
          i32.const 16
          i32.add
          i32.load
          call $_proxy_log
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
          global.set $12
          return
        end ;; $block_1
        call $core::result::unwrap_failed::h1d6d220c53d3f336
        unreachable
      end ;; $block_0
      i32.const 1053032
      call $core::panicking::panic::h13421bef49a1c627
      unreachable
    end ;; $block
    unreachable
    unreachable
    )
  
  (func $<&T_as_core::fmt::Display>::fmt::h3f145b24ed245033 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    global.get $12
    i32.const 32
    i32.sub
    local.tee $2
    global.set $12
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
    call $core::fmt::write::h2bc7cdeb5edd0d71
    local.set $1
    local.get $2
    i32.const 32
    i32.add
    global.set $12
    local.get $1
    )
  
  (func $core::fmt::write::h2bc7cdeb5edd0d71 (type $1)
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
    (local $15 i32)
    (local $16 i32)
    (local $17 i32)
    (local $18 i32)
    (local $19 i32)
    global.get $12
    i32.const 64
    i32.sub
    local.tee $3
    global.set $12
    local.get $3
    i32.const 8
    i32.add
    i32.const 28
    i32.add
    local.tee $4
    local.get $1
    i32.store
    local.get $3
    i32.const 52
    i32.add
    local.tee $5
    local.get $2
    i32.const 20
    i32.add
    i32.load
    local.tee $1
    i32.store
    local.get $3
    i32.const 3
    i32.store8 offset=56
    local.get $3
    i32.const 8
    i32.add
    i32.const 36
    i32.add
    local.tee $6
    local.get $2
    i32.load offset=16
    local.tee $7
    local.get $1
    i32.const 3
    i32.shl
    local.tee $8
    i32.add
    i32.store
    local.get $3
    i64.const 137438953472
    i64.store offset=8
    local.get $3
    local.get $0
    i32.store offset=32
    i32.const 0
    local.set $1
    local.get $3
    i32.const 0
    i32.store offset=24
    local.get $3
    i32.const 0
    i32.store offset=16
    local.get $3
    local.get $7
    i32.store offset=48
    local.get $3
    local.get $7
    i32.store offset=40
    local.get $2
    i32.load offset=4
    i32.const 3
    i32.shl
    local.set $9
    local.get $2
    i32.load
    local.set $10
    block $block
      block $block_0
        block $block_1
          block $block_2
            block $block_3
              block $block_4
                block $block_5
                  block $block_6
                    block $block_7
                      local.get $2
                      i32.load offset=8
                      local.tee $0
                      i32.eqz
                      br_if $block_7
                      local.get $0
                      i32.const 28
                      i32.add
                      local.set $7
                      local.get $2
                      i32.const 12
                      i32.add
                      i32.load
                      i32.const 36
                      i32.mul
                      local.set $2
                      local.get $3
                      i32.const 32
                      i32.add
                      local.set $11
                      local.get $3
                      i32.const 56
                      i32.add
                      local.set $12
                      local.get $3
                      i32.const 48
                      i32.add
                      local.set $13
                      local.get $3
                      i32.const 8
                      i32.add
                      i32.const 8
                      i32.add
                      local.set $14
                      local.get $3
                      i32.const 8
                      i32.add
                      i32.const 20
                      i32.add
                      local.set $15
                      local.get $3
                      i32.const 24
                      i32.add
                      local.set $16
                      local.get $3
                      i32.const 40
                      i32.add
                      local.set $17
                      loop $loop
                        local.get $2
                        i32.eqz
                        br_if $block_6
                        local.get $9
                        local.get $1
                        i32.eq
                        br_if $block_3
                        local.get $11
                        i32.load
                        local.get $10
                        local.get $1
                        i32.add
                        local.tee $8
                        i32.load
                        local.get $8
                        i32.const 4
                        i32.add
                        i32.load
                        local.get $4
                        i32.load
                        i32.load offset=12
                        call_indirect $10 (type $1)
                        br_if $block_4
                        local.get $12
                        local.get $0
                        i32.load8_u offset=32
                        i32.store8
                        local.get $3
                        local.get $0
                        i32.load offset=8
                        i32.store offset=12
                        local.get $3
                        local.get $0
                        i32.load offset=12
                        i32.store offset=8
                        i32.const 0
                        local.set $8
                        block $block_8
                          block $block_9
                            block $block_10
                              block $block_11
                                local.get $0
                                i32.load offset=24
                                local.tee $18
                                i32.const 1
                                i32.eq
                                br_if $block_11
                                block $block_12
                                  local.get $18
                                  i32.const 3
                                  i32.eq
                                  br_if $block_12
                                  local.get $18
                                  i32.const 2
                                  i32.ne
                                  br_if $block_10
                                  local.get $17
                                  i32.load
                                  local.tee $19
                                  local.get $6
                                  i32.load
                                  i32.eq
                                  br_if $block_12
                                  local.get $17
                                  local.get $19
                                  i32.const 8
                                  i32.add
                                  i32.store
                                  local.get $19
                                  i32.load offset=4
                                  i32.const 5
                                  i32.ne
                                  br_if $block_8
                                  local.get $19
                                  i32.load
                                  i32.load
                                  local.set $18
                                  br $block_9
                                end ;; $block_12
                                br $block_8
                              end ;; $block_11
                              local.get $7
                              i32.load
                              local.tee $19
                              local.get $5
                              i32.load
                              local.tee $18
                              i32.ge_u
                              br_if $block_0
                              local.get $13
                              i32.load
                              local.get $19
                              i32.const 3
                              i32.shl
                              i32.add
                              local.tee $19
                              i32.load offset=4
                              i32.const 5
                              i32.ne
                              br_if $block_8
                              local.get $19
                              i32.load
                              i32.load
                              local.set $18
                              br $block_9
                            end ;; $block_10
                            local.get $7
                            i32.load
                            local.set $18
                          end ;; $block_9
                          i32.const 1
                          local.set $8
                        end ;; $block_8
                        local.get $3
                        i32.const 8
                        i32.add
                        i32.const 12
                        i32.add
                        local.get $18
                        i32.store
                        local.get $14
                        local.get $8
                        i32.store
                        i32.const 0
                        local.set $8
                        block $block_13
                          block $block_14
                            block $block_15
                              block $block_16
                                local.get $0
                                i32.load offset=16
                                local.tee $18
                                i32.const 1
                                i32.eq
                                br_if $block_16
                                block $block_17
                                  local.get $18
                                  i32.const 3
                                  i32.eq
                                  br_if $block_17
                                  local.get $18
                                  i32.const 2
                                  i32.ne
                                  br_if $block_15
                                  local.get $17
                                  i32.load
                                  local.tee $19
                                  local.get $6
                                  i32.load
                                  i32.eq
                                  br_if $block_17
                                  local.get $17
                                  local.get $19
                                  i32.const 8
                                  i32.add
                                  i32.store
                                  local.get $19
                                  i32.load offset=4
                                  i32.const 5
                                  i32.ne
                                  br_if $block_13
                                  local.get $19
                                  i32.load
                                  i32.load
                                  local.set $18
                                  br $block_14
                                end ;; $block_17
                                br $block_13
                              end ;; $block_16
                              local.get $7
                              i32.const -8
                              i32.add
                              i32.load
                              local.tee $19
                              local.get $5
                              i32.load
                              local.tee $18
                              i32.ge_u
                              br_if $block
                              local.get $13
                              i32.load
                              local.get $19
                              i32.const 3
                              i32.shl
                              i32.add
                              local.tee $19
                              i32.load offset=4
                              i32.const 5
                              i32.ne
                              br_if $block_13
                              local.get $19
                              i32.load
                              i32.load
                              local.set $18
                              br $block_14
                            end ;; $block_15
                            local.get $7
                            i32.const -8
                            i32.add
                            i32.load
                            local.set $18
                          end ;; $block_14
                          i32.const 1
                          local.set $8
                        end ;; $block_13
                        local.get $15
                        local.get $18
                        i32.store
                        local.get $16
                        local.get $8
                        i32.store
                        block $block_18
                          block $block_19
                            local.get $0
                            i32.load
                            i32.const 1
                            i32.ne
                            br_if $block_19
                            local.get $7
                            i32.const -24
                            i32.add
                            i32.load
                            local.tee $8
                            local.get $5
                            i32.load
                            local.tee $18
                            i32.ge_u
                            br_if $block_2
                            local.get $13
                            i32.load
                            local.get $8
                            i32.const 3
                            i32.shl
                            i32.add
                            local.set $8
                            br $block_18
                          end ;; $block_19
                          local.get $17
                          i32.load
                          local.tee $8
                          local.get $6
                          i32.load
                          i32.eq
                          br_if $block_1
                          local.get $17
                          local.get $8
                          i32.const 8
                          i32.add
                          i32.store
                        end ;; $block_18
                        local.get $0
                        i32.const 36
                        i32.add
                        local.set $0
                        local.get $2
                        i32.const -36
                        i32.add
                        local.set $2
                        local.get $7
                        i32.const 36
                        i32.add
                        local.set $7
                        local.get $1
                        i32.const 8
                        i32.add
                        local.set $1
                        local.get $8
                        i32.load
                        local.get $3
                        i32.const 8
                        i32.add
                        local.get $8
                        i32.const 4
                        i32.add
                        i32.load
                        call_indirect $10 (type $2)
                        i32.eqz
                        br_if $loop
                        br $block_4
                      end ;; $loop
                    end ;; $block_7
                    local.get $3
                    i32.const 32
                    i32.add
                    local.set $17
                    local.get $9
                    local.set $1
                    local.get $10
                    local.set $0
                    loop $loop_0
                      local.get $8
                      i32.eqz
                      br_if $block_5
                      local.get $1
                      i32.eqz
                      br_if $block_3
                      local.get $17
                      i32.load
                      local.get $0
                      i32.load
                      local.get $0
                      i32.const 4
                      i32.add
                      i32.load
                      local.get $4
                      i32.load
                      i32.load offset=12
                      call_indirect $10 (type $1)
                      br_if $block_4
                      local.get $8
                      i32.const -8
                      i32.add
                      local.set $8
                      local.get $1
                      i32.const -8
                      i32.add
                      local.set $1
                      local.get $0
                      i32.const 8
                      i32.add
                      local.set $0
                      local.get $7
                      i32.load
                      local.set $2
                      local.get $7
                      i32.load offset=4
                      local.set $18
                      local.get $7
                      i32.const 8
                      i32.add
                      local.set $7
                      local.get $2
                      local.get $3
                      i32.const 8
                      i32.add
                      local.get $18
                      call_indirect $10 (type $2)
                      i32.eqz
                      br_if $loop_0
                      br $block_4
                    end ;; $loop_0
                  end ;; $block_6
                  local.get $10
                  local.get $1
                  i32.add
                  local.set $0
                end ;; $block_5
                local.get $0
                local.get $10
                local.get $9
                i32.add
                i32.eq
                br_if $block_3
                local.get $3
                i32.const 32
                i32.add
                i32.load
                local.get $0
                i32.load
                local.get $0
                i32.load offset=4
                local.get $3
                i32.const 36
                i32.add
                i32.load
                i32.load offset=12
                call_indirect $10 (type $1)
                i32.eqz
                br_if $block_3
              end ;; $block_4
              local.get $3
              i32.const 64
              i32.add
              global.set $12
              i32.const 1
              return
            end ;; $block_3
            local.get $3
            i32.const 64
            i32.add
            global.set $12
            i32.const 0
            return
          end ;; $block_2
          i32.const 1053456
          local.get $8
          local.get $18
          call $core::panicking::panic_bounds_check::h34b80e64d41db052
          unreachable
        end ;; $block_1
        i32.const 1053344
        call $core::panicking::panic::h13421bef49a1c627
        unreachable
      end ;; $block_0
      i32.const 1053440
      local.get $19
      local.get $18
      call $core::panicking::panic_bounds_check::h34b80e64d41db052
      unreachable
    end ;; $block
    i32.const 1053440
    local.get $19
    local.get $18
    call $core::panicking::panic_bounds_check::h34b80e64d41db052
    unreachable
    )
  
  (func $__rust_realloc (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    local.get $0
    local.get $1
    call $__rdl_realloc
    )
  
  (func $__rust_dealloc (type $5)
    (param $0 i32)
    local.get $0
    call $__rdl_dealloc
    )
  
  (func $core::panicking::panic::h13421bef49a1c627 (type $5)
    (param $0 i32)
    (local $1 i32)
    (local $2 i64)
    (local $3 i64)
    (local $4 i64)
    global.get $12
    i32.const 48
    i32.sub
    local.tee $1
    global.set $12
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
    local.get $4
    i64.store offset=24
    local.get $1
    i32.const 1049000
    i32.store offset=16
    local.get $1
    i64.const 1
    i64.store offset=4 align=4
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
    call $core::panicking::panic_fmt::h74ee8034b317ceed
    unreachable
    )
  
  (func $__post_instantiate (type $4)
    (local $0 i32)
    i32.const 0
    i32.const 0
    i32.load offset=1055132
    local.tee $0
    i32.const 1
    local.get $0
    select
    i32.store offset=1055132
    block $block
      local.get $0
      i32.eqz
      br_if $block
      block $block_0
        local.get $0
        i32.const 1
        i32.ne
        br_if $block_0
        loop $loop
          i32.const 0
          i32.load offset=1055132
          i32.const 1
          i32.eq
          br_if $loop
        end ;; $loop
      end ;; $block_0
      call $core::result::unwrap_failed::h53c13222d410a969
      unreachable
    end ;; $block
    i32.const 0
    i32.const 1053000
    i32.store offset=1053084
    i32.const 0
    i32.const 1049000
    i32.store offset=1053080
    i32.const 0
    i32.const 2
    i32.store offset=1055132
    i32.const 0
    i32.const 5
    i32.store offset=1055128
    )
  
  (func $_malloc (type $6)
    (param $0 i32)
    (result i32)
    block $block
      block $block_0
        local.get $0
        i32.const -1
        i32.le_s
        br_if $block_0
        block $block_1
          local.get $0
          i32.eqz
          br_if $block_1
          local.get $0
          i32.const 1
          call $__rust_alloc
          local.tee $0
          i32.eqz
          br_if $block
          local.get $0
          return
        end ;; $block_1
        i32.const 1
        return
      end ;; $block_0
      call $<alloc::raw_vec::RawVec<T_2c__A>>::allocate_in::_7b__7b_closure_7d__7d_::haa1dbf56fe1aeb63
      unreachable
    end ;; $block
    unreachable
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
  
  (func $<alloc::raw_vec::RawVec<T_2c__A>>::allocate_in::_7b__7b_closure_7d__7d_::haa1dbf56fe1aeb63 (type $4)
    call $alloc::raw_vec::capacity_overflow::he82aaa6e8f2774ef
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
  
  (func $alloc::raw_vec::capacity_overflow::he82aaa6e8f2774ef (type $4)
    i32.const 1053136
    call $core::panicking::panic::h13421bef49a1c627
    unreachable
    )
  
  (func $<&mut_W_as_core::fmt::Write>::write_str::hccd6f22edec2e385 (type $1)
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
            block $block_3
              block $block_4
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
                i32.ge_u
                br_if $block_4
                local.get $4
                local.get $2
                i32.add
                local.tee $5
                local.get $4
                i32.lt_u
                br_if $block_1
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
                br_if $block_1
                local.get $3
                i32.eqz
                br_if $block_3
                local.get $0
                i32.load
                local.get $4
                call $__rust_realloc
                local.tee $3
                i32.eqz
                br_if $block_2
                br $block_0
              end ;; $block_4
              local.get $0
              i32.load
              local.set $3
              br $block
            end ;; $block_3
            local.get $4
            i32.const 1
            call $__rust_alloc
            local.tee $3
            br_if $block_0
          end ;; $block_2
          unreachable
          unreachable
        end ;; $block_1
        call $alloc::raw_vec::capacity_overflow::he82aaa6e8f2774ef
        unreachable
      end ;; $block_0
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
    end ;; $block
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
    )
  
  (func $<&mut_W_as_core::fmt::Write>::write_char::hc45ff4c6ed2a2042 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    global.get $12
    i32.const 16
    i32.sub
    local.tee $2
    global.set $12
    local.get $0
    i32.load
    local.set $0
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
                          block $block_10
                            block $block_11
                              local.get $1
                              i32.const 128
                              i32.ge_u
                              br_if $block_11
                              local.get $0
                              i32.load offset=8
                              local.tee $3
                              local.get $0
                              i32.const 4
                              i32.add
                              i32.load
                              i32.ne
                              br_if $block_10
                              local.get $3
                              i32.const 1
                              i32.add
                              local.tee $4
                              local.get $3
                              i32.lt_u
                              br_if $block_0
                              local.get $3
                              i32.const 1
                              i32.shl
                              local.tee $5
                              local.get $4
                              local.get $4
                              local.get $5
                              i32.lt_u
                              select
                              local.tee $5
                              i32.const 0
                              i32.lt_s
                              br_if $block_0
                              local.get $3
                              i32.eqz
                              br_if $block_7
                              local.get $0
                              i32.load
                              local.get $5
                              call $__rust_realloc
                              local.tee $4
                              br_if $block_6
                              br $block
                            end ;; $block_11
                            local.get $2
                            i32.const 0
                            i32.store offset=12
                            local.get $1
                            i32.const 2048
                            i32.ge_u
                            br_if $block_9
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
                            br $block_8
                          end ;; $block_10
                          local.get $0
                          i32.load
                          local.set $4
                          br $block_5
                        end ;; $block_9
                        block $block_12
                          local.get $1
                          i32.const 65535
                          i32.gt_u
                          br_if $block_12
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
                          br $block_8
                        end ;; $block_12
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
                      end ;; $block_8
                      block $block_13
                        local.get $0
                        i32.const 4
                        i32.add
                        i32.load
                        local.tee $4
                        local.get $0
                        i32.const 8
                        i32.add
                        i32.load
                        local.tee $3
                        i32.sub
                        local.get $1
                        i32.ge_u
                        br_if $block_13
                        local.get $3
                        local.get $1
                        i32.add
                        local.tee $5
                        local.get $3
                        i32.lt_u
                        br_if $block_0
                        local.get $4
                        i32.const 1
                        i32.shl
                        local.tee $3
                        local.get $5
                        local.get $5
                        local.get $3
                        i32.lt_u
                        select
                        local.tee $3
                        i32.const 0
                        i32.lt_s
                        br_if $block_0
                        local.get $4
                        i32.eqz
                        br_if $block_4
                        local.get $0
                        i32.load
                        local.get $3
                        call $__rust_realloc
                        local.tee $4
                        br_if $block_3
                        br $block
                      end ;; $block_13
                      local.get $0
                      i32.load
                      local.set $4
                      br $block_2
                    end ;; $block_7
                    local.get $5
                    i32.const 1
                    call $__rust_alloc
                    local.tee $4
                    i32.eqz
                    br_if $block
                  end ;; $block_6
                  local.get $0
                  local.get $4
                  i32.store
                  local.get $0
                  i32.const 4
                  i32.add
                  local.get $5
                  i32.store
                  local.get $0
                  i32.const 8
                  i32.add
                  i32.load
                  local.set $3
                end ;; $block_5
                local.get $4
                local.get $3
                i32.add
                local.get $1
                i32.store8
                local.get $0
                i32.const 8
                i32.add
                local.tee $0
                local.get $0
                i32.load
                i32.const 1
                i32.add
                i32.store
                br $block_1
              end ;; $block_4
              local.get $3
              i32.const 1
              call $__rust_alloc
              local.tee $4
              i32.eqz
              br_if $block
            end ;; $block_3
            local.get $0
            local.get $4
            i32.store
            local.get $0
            i32.const 4
            i32.add
            local.get $3
            i32.store
            local.get $0
            i32.const 8
            i32.add
            i32.load
            local.set $3
          end ;; $block_2
          local.get $0
          i32.const 8
          i32.add
          local.get $3
          local.get $1
          i32.add
          i32.store
          local.get $4
          local.get $3
          i32.add
          local.get $2
          i32.const 12
          i32.add
          local.get $1
          call $memcpy
          drop
        end ;; $block_1
        local.get $2
        i32.const 16
        i32.add
        global.set $12
        i32.const 0
        return
      end ;; $block_0
      call $alloc::raw_vec::capacity_overflow::he82aaa6e8f2774ef
      unreachable
    end ;; $block
    unreachable
    unreachable
    )
  
  (func $<&mut_W_as_core::fmt::Write>::write_fmt::hc3368130906df163 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    global.get $12
    i32.const 32
    i32.sub
    local.tee $2
    global.set $12
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
    i32.const 1053056
    local.get $2
    i32.const 8
    i32.add
    call $core::fmt::write::h2bc7cdeb5edd0d71
    local.set $1
    local.get $2
    i32.const 32
    i32.add
    global.set $12
    local.get $1
    )
  
  (func $<log::NopLogger_as_log::Log>::enabled::h1a8020a0a9c8ab7b (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    i32.const 0
    )
  
  (func $<log::NopLogger_as_log::Log>::log::h3310d0bba2c563ff (type $0)
    (param $0 i32)
    (param $1 i32)
    )
  
  (func $<log::NopLogger_as_log::Log>::flush::hd7e64e1b27d09f04 (type $5)
    (param $0 i32)
    )
  
  (func $core::fmt::Formatter::pad::hfe31e8860a6945ec (type $1)
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
    global.get $12
    i32.const 16
    i32.sub
    local.tee $3
    global.set $12
    local.get $0
    i32.load offset=16
    local.set $4
    block $block
      block $block_0
        block $block_1
          block $block_2
            block $block_3
              block $block_4
                block $block_5
                  block $block_6
                    block $block_7
                      local.get $0
                      i32.load offset=8
                      local.tee $5
                      i32.const 1
                      i32.ne
                      br_if $block_7
                      local.get $4
                      br_if $block_6
                      br $block_5
                    end ;; $block_7
                    local.get $4
                    i32.eqz
                    br_if $block
                  end ;; $block_6
                  block $block_8
                    local.get $2
                    i32.eqz
                    br_if $block_8
                    local.get $1
                    local.get $2
                    i32.add
                    local.set $6
                    local.get $0
                    i32.const 20
                    i32.add
                    i32.load
                    i32.const -1
                    i32.xor
                    local.set $7
                    i32.const 0
                    local.set $8
                    local.get $1
                    local.set $4
                    local.get $1
                    local.set $9
                    block $block_9
                      block $block_10
                        loop $loop
                          local.get $4
                          i32.const 1
                          i32.add
                          local.set $10
                          block $block_11
                            block $block_12
                              local.get $4
                              i32.load8_s
                              local.tee $11
                              i32.const 0
                              i32.lt_s
                              br_if $block_12
                              local.get $11
                              i32.const 255
                              i32.and
                              local.set $11
                              local.get $10
                              local.set $4
                              local.get $7
                              i32.const 1
                              i32.add
                              local.tee $7
                              br_if $block_11
                              br $block_10
                            end ;; $block_12
                            block $block_13
                              block $block_14
                                block $block_15
                                  local.get $10
                                  local.get $6
                                  i32.eq
                                  br_if $block_15
                                  local.get $10
                                  i32.load8_u
                                  i32.const 63
                                  i32.and
                                  local.set $12
                                  local.get $4
                                  i32.const 2
                                  i32.add
                                  local.tee $4
                                  local.set $10
                                  local.get $11
                                  i32.const 31
                                  i32.and
                                  local.set $13
                                  local.get $11
                                  i32.const 255
                                  i32.and
                                  local.tee $11
                                  i32.const 224
                                  i32.lt_u
                                  br_if $block_14
                                  br $block_13
                                end ;; $block_15
                                i32.const 0
                                local.set $12
                                local.get $6
                                local.set $4
                                local.get $11
                                i32.const 31
                                i32.and
                                local.set $13
                                local.get $11
                                i32.const 255
                                i32.and
                                local.tee $11
                                i32.const 224
                                i32.ge_u
                                br_if $block_13
                              end ;; $block_14
                              local.get $12
                              local.get $13
                              i32.const 6
                              i32.shl
                              i32.or
                              local.set $11
                              local.get $10
                              local.set $4
                              local.get $7
                              i32.const 1
                              i32.add
                              local.tee $7
                              br_if $block_11
                              br $block_10
                            end ;; $block_13
                            block $block_16
                              block $block_17
                                block $block_18
                                  local.get $4
                                  local.get $6
                                  i32.eq
                                  br_if $block_18
                                  local.get $4
                                  i32.const 1
                                  i32.add
                                  local.tee $10
                                  local.set $14
                                  local.get $4
                                  i32.load8_u
                                  i32.const 63
                                  i32.and
                                  local.get $12
                                  i32.const 6
                                  i32.shl
                                  i32.or
                                  local.set $12
                                  local.get $11
                                  i32.const 240
                                  i32.lt_u
                                  br_if $block_17
                                  br $block_16
                                end ;; $block_18
                                local.get $6
                                local.set $14
                                i32.const 0
                                local.get $12
                                i32.const 6
                                i32.shl
                                i32.or
                                local.set $12
                                local.get $11
                                i32.const 240
                                i32.ge_u
                                br_if $block_16
                              end ;; $block_17
                              local.get $12
                              local.get $13
                              i32.const 12
                              i32.shl
                              i32.or
                              local.set $11
                              local.get $10
                              local.set $4
                              local.get $7
                              i32.const 1
                              i32.add
                              local.tee $7
                              br_if $block_11
                              br $block_10
                            end ;; $block_16
                            block $block_19
                              block $block_20
                                local.get $14
                                local.get $6
                                i32.eq
                                br_if $block_20
                                local.get $14
                                i32.const 1
                                i32.add
                                local.set $4
                                local.get $14
                                i32.load8_u
                                i32.const 63
                                i32.and
                                local.set $11
                                br $block_19
                              end ;; $block_20
                              i32.const 0
                              local.set $11
                              local.get $10
                              local.set $4
                            end ;; $block_19
                            local.get $12
                            i32.const 6
                            i32.shl
                            local.get $13
                            i32.const 18
                            i32.shl
                            i32.const 1835008
                            i32.and
                            i32.or
                            local.get $11
                            i32.or
                            local.tee $11
                            i32.const 1114112
                            i32.eq
                            br_if $block_9
                            local.get $7
                            i32.const 1
                            i32.add
                            local.tee $7
                            i32.eqz
                            br_if $block_10
                          end ;; $block_11
                          local.get $8
                          local.get $9
                          i32.sub
                          local.get $4
                          i32.add
                          local.set $8
                          local.get $4
                          local.set $9
                          local.get $6
                          local.get $4
                          i32.ne
                          br_if $loop
                          br $block_9
                        end ;; $loop
                      end ;; $block_10
                      local.get $11
                      i32.const 1114112
                      i32.eq
                      br_if $block_9
                      block $block_21
                        block $block_22
                          local.get $8
                          i32.eqz
                          br_if $block_22
                          local.get $8
                          local.get $2
                          i32.eq
                          br_if $block_22
                          i32.const 0
                          local.set $4
                          local.get $8
                          local.get $2
                          i32.ge_u
                          br_if $block_21
                          local.get $1
                          local.get $8
                          i32.add
                          i32.load8_s
                          i32.const -64
                          i32.lt_s
                          br_if $block_21
                        end ;; $block_22
                        local.get $1
                        local.set $4
                      end ;; $block_21
                      local.get $8
                      local.get $2
                      local.get $4
                      select
                      local.set $2
                      local.get $4
                      local.get $1
                      local.get $4
                      select
                      local.set $1
                    end ;; $block_9
                    local.get $5
                    i32.eqz
                    br_if $block_4
                    br $block_5
                  end ;; $block_8
                  i32.const 0
                  local.set $2
                  local.get $5
                  i32.eqz
                  br_if $block_4
                end ;; $block_5
                i32.const 0
                local.set $10
                block $block_23
                  local.get $2
                  i32.eqz
                  br_if $block_23
                  local.get $2
                  local.set $11
                  local.get $1
                  local.set $4
                  loop $loop_0
                    local.get $10
                    local.get $4
                    i32.load8_u
                    i32.const 192
                    i32.and
                    i32.const 128
                    i32.eq
                    i32.add
                    local.set $10
                    local.get $4
                    i32.const 1
                    i32.add
                    local.set $4
                    local.get $11
                    i32.const -1
                    i32.add
                    local.tee $11
                    br_if $loop_0
                  end ;; $loop_0
                end ;; $block_23
                local.get $2
                local.get $10
                i32.sub
                local.get $0
                i32.const 12
                i32.add
                i32.load
                local.tee $7
                i32.ge_u
                br_if $block_3
                i32.const 0
                local.set $10
                block $block_24
                  local.get $2
                  i32.eqz
                  br_if $block_24
                  i32.const 0
                  local.set $10
                  local.get $2
                  local.set $11
                  local.get $1
                  local.set $4
                  loop $loop_1
                    local.get $10
                    local.get $4
                    i32.load8_u
                    i32.const 192
                    i32.and
                    i32.const 128
                    i32.eq
                    i32.add
                    local.set $10
                    local.get $4
                    i32.const 1
                    i32.add
                    local.set $4
                    local.get $11
                    i32.const -1
                    i32.add
                    local.tee $11
                    br_if $loop_1
                  end ;; $loop_1
                end ;; $block_24
                local.get $10
                local.get $2
                i32.sub
                local.get $7
                i32.add
                local.set $11
                i32.const 0
                local.get $0
                i32.load8_u offset=48
                local.tee $4
                local.get $4
                i32.const 3
                i32.eq
                select
                i32.const 3
                i32.and
                local.tee $4
                i32.eqz
                br_if $block_2
                local.get $4
                i32.const 2
                i32.eq
                br_if $block_1
                i32.const 0
                local.set $7
                br $block_0
              end ;; $block_4
              local.get $0
              i32.load offset=24
              local.get $1
              local.get $2
              local.get $0
              i32.const 28
              i32.add
              i32.load
              i32.load offset=12
              call_indirect $10 (type $1)
              local.set $4
              local.get $3
              i32.const 16
              i32.add
              global.set $12
              local.get $4
              return
            end ;; $block_3
            local.get $0
            i32.load offset=24
            local.get $1
            local.get $2
            local.get $0
            i32.const 28
            i32.add
            i32.load
            i32.load offset=12
            call_indirect $10 (type $1)
            local.set $4
            local.get $3
            i32.const 16
            i32.add
            global.set $12
            local.get $4
            return
          end ;; $block_2
          local.get $11
          local.set $7
          i32.const 0
          local.set $11
          br $block_0
        end ;; $block_1
        local.get $11
        i32.const 1
        i32.add
        i32.const 1
        i32.shr_u
        local.set $7
        local.get $11
        i32.const 1
        i32.shr_u
        local.set $11
      end ;; $block_0
      local.get $3
      i32.const 0
      i32.store offset=12
      block $block_25
        block $block_26
          local.get $0
          i32.load offset=4
          local.tee $4
          i32.const 127
          i32.gt_u
          br_if $block_26
          local.get $3
          local.get $4
          i32.store8 offset=12
          i32.const 1
          local.set $10
          br $block_25
        end ;; $block_26
        block $block_27
          local.get $4
          i32.const 2047
          i32.gt_u
          br_if $block_27
          local.get $3
          local.get $4
          i32.const 63
          i32.and
          i32.const 128
          i32.or
          i32.store8 offset=13
          local.get $3
          local.get $4
          i32.const 6
          i32.shr_u
          i32.const 31
          i32.and
          i32.const 192
          i32.or
          i32.store8 offset=12
          i32.const 2
          local.set $10
          br $block_25
        end ;; $block_27
        block $block_28
          local.get $4
          i32.const 65535
          i32.gt_u
          br_if $block_28
          local.get $3
          local.get $4
          i32.const 63
          i32.and
          i32.const 128
          i32.or
          i32.store8 offset=14
          local.get $3
          local.get $4
          i32.const 6
          i32.shr_u
          i32.const 63
          i32.and
          i32.const 128
          i32.or
          i32.store8 offset=13
          local.get $3
          local.get $4
          i32.const 12
          i32.shr_u
          i32.const 15
          i32.and
          i32.const 224
          i32.or
          i32.store8 offset=12
          i32.const 3
          local.set $10
          br $block_25
        end ;; $block_28
        local.get $3
        local.get $4
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=15
        local.get $3
        local.get $4
        i32.const 18
        i32.shr_u
        i32.const 240
        i32.or
        i32.store8 offset=12
        local.get $3
        local.get $4
        i32.const 6
        i32.shr_u
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=14
        local.get $3
        local.get $4
        i32.const 12
        i32.shr_u
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=13
        i32.const 4
        local.set $10
      end ;; $block_25
      i32.const -1
      local.set $4
      block $block_29
        block $block_30
          block $block_31
            loop $loop_2
              local.get $4
              i32.const 1
              i32.add
              local.tee $4
              local.get $11
              i32.ge_u
              br_if $block_31
              local.get $0
              i32.const 24
              i32.add
              i32.load
              local.get $3
              i32.const 12
              i32.add
              local.get $10
              local.get $0
              i32.const 28
              i32.add
              i32.load
              i32.load offset=12
              call_indirect $10 (type $1)
              i32.eqz
              br_if $loop_2
              br $block_30
            end ;; $loop_2
          end ;; $block_31
          local.get $0
          i32.const 24
          i32.add
          local.tee $11
          i32.load
          local.get $1
          local.get $2
          local.get $0
          i32.const 28
          i32.add
          local.tee $0
          i32.load
          i32.load offset=12
          call_indirect $10 (type $1)
          i32.eqz
          br_if $block_29
        end ;; $block_30
        local.get $3
        i32.const 16
        i32.add
        global.set $12
        i32.const 1
        return
      end ;; $block_29
      i32.const -1
      local.set $4
      block $block_32
        loop $loop_3
          local.get $4
          i32.const 1
          i32.add
          local.tee $4
          local.get $7
          i32.ge_u
          br_if $block_32
          local.get $11
          i32.load
          local.get $3
          i32.const 12
          i32.add
          local.get $10
          local.get $0
          i32.load
          i32.load offset=12
          call_indirect $10 (type $1)
          i32.eqz
          br_if $loop_3
        end ;; $loop_3
        local.get $3
        i32.const 16
        i32.add
        global.set $12
        i32.const 1
        return
      end ;; $block_32
      local.get $3
      i32.const 16
      i32.add
      global.set $12
      i32.const 0
      return
    end ;; $block
    local.get $0
    i32.load offset=24
    local.get $1
    local.get $2
    local.get $0
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect $10 (type $1)
    local.set $4
    local.get $3
    i32.const 16
    i32.add
    global.set $12
    local.get $4
    )
  
  (func $<core::fmt::builders::PadAdapter<'_>_as_core::fmt::Write>::write_str::heb6658a92b0d254f (type $1)
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
    (local $15 i32)
    global.get $12
    i32.const 48
    i32.sub
    local.tee $3
    global.set $12
    block $block
      local.get $2
      i32.eqz
      br_if $block
      local.get $3
      i32.const 40
      i32.add
      local.set $4
      local.get $0
      i32.const 8
      i32.add
      local.set $5
      local.get $3
      i32.const 32
      i32.add
      local.set $6
      local.get $3
      i32.const 28
      i32.add
      local.set $7
      local.get $3
      i32.const 36
      i32.add
      local.set $8
      local.get $0
      i32.const 4
      i32.add
      local.set $9
      block $block_0
        block $block_1
          block $block_2
            block $block_3
              loop $loop
                block $block_4
                  local.get $5
                  i32.load8_u
                  i32.eqz
                  br_if $block_4
                  local.get $0
                  i32.load
                  i32.const 1052912
                  i32.const 4
                  local.get $9
                  i32.load
                  i32.load offset=12
                  call_indirect $10 (type $1)
                  br_if $block_3
                end ;; $block_4
                local.get $4
                i32.const 10
                i32.store
                local.get $6
                i64.const 4294967306
                i64.store
                local.get $7
                local.get $2
                i32.store
                local.get $3
                i32.const 16
                i32.add
                i32.const 8
                i32.add
                local.tee $10
                i32.const 0
                i32.store
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
                call $core::slice::memchr::memchr::h2712233316cf809b
                block $block_5
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
                          local.set $11
                          loop $loop_0
                            local.get $10
                            local.get $11
                            local.get $10
                            i32.load
                            i32.add
                            i32.const 1
                            i32.add
                            local.tee $11
                            i32.store
                            block $block_10
                              block $block_11
                                local.get $11
                                local.get $8
                                i32.load
                                local.tee $12
                                i32.ge_u
                                br_if $block_11
                                local.get $3
                                i32.load offset=20
                                local.set $13
                                local.get $7
                                i32.load
                                local.tee $14
                                local.get $11
                                i32.ge_u
                                br_if $block_10
                                br $block_8
                              end ;; $block_11
                              block $block_12
                                local.get $3
                                i32.load offset=20
                                local.tee $13
                                local.get $11
                                i32.lt_u
                                br_if $block_12
                                local.get $12
                                i32.const 5
                                i32.ge_u
                                br_if $block_6
                                local.get $3
                                i32.load offset=16
                                local.get $11
                                local.get $12
                                i32.sub
                                local.tee $15
                                i32.add
                                local.tee $14
                                local.get $4
                                i32.eq
                                br_if $block_7
                                local.get $14
                                local.get $4
                                local.get $12
                                call $memcmp
                                i32.eqz
                                br_if $block_7
                              end ;; $block_12
                              local.get $7
                              i32.load
                              local.tee $14
                              local.get $11
                              i32.lt_u
                              br_if $block_8
                            end ;; $block_10
                            local.get $13
                            local.get $14
                            i32.lt_u
                            br_if $block_8
                            local.get $3
                            local.get $3
                            i32.const 16
                            i32.add
                            local.get $12
                            i32.add
                            i32.const 23
                            i32.add
                            i32.load8_u
                            local.get $3
                            i32.load offset=16
                            local.get $11
                            i32.add
                            local.get $14
                            local.get $11
                            i32.sub
                            call $core::slice::memchr::memchr::h2712233316cf809b
                            local.get $3
                            i32.load offset=4
                            local.set $11
                            local.get $3
                            i32.load
                            i32.const 1
                            i32.eq
                            br_if $loop_0
                          end ;; $loop_0
                        end ;; $block_9
                        local.get $10
                        local.get $7
                        i32.load
                        i32.store
                      end ;; $block_8
                      local.get $5
                      i32.const 0
                      i32.store8
                      local.get $2
                      local.set $11
                      br $block_5
                    end ;; $block_7
                    local.get $5
                    i32.const 1
                    i32.store8
                    local.get $15
                    i32.const 1
                    i32.add
                    local.set $11
                    br $block_5
                  end ;; $block_6
                  local.get $12
                  i32.const 4
                  call $core::slice::slice_index_len_fail::h7a4ef59ca849a060
                  unreachable
                end ;; $block_5
                local.get $9
                i32.load
                local.set $14
                local.get $0
                i32.load
                local.set $12
                block $block_13
                  local.get $11
                  i32.eqz
                  local.get $2
                  local.get $11
                  i32.eq
                  i32.or
                  local.tee $10
                  br_if $block_13
                  local.get $2
                  local.get $11
                  i32.le_u
                  br_if $block_1
                  local.get $1
                  local.get $11
                  i32.add
                  i32.load8_s
                  i32.const -65
                  i32.le_s
                  br_if $block_1
                end ;; $block_13
                local.get $12
                local.get $1
                local.get $11
                local.get $14
                i32.load offset=12
                call_indirect $10 (type $1)
                br_if $block_3
                block $block_14
                  local.get $10
                  i32.eqz
                  br_if $block_14
                  local.get $1
                  local.get $11
                  i32.add
                  local.set $1
                  local.get $2
                  local.get $11
                  i32.sub
                  local.tee $2
                  br_if $loop
                  br $block_2
                end ;; $block_14
                local.get $2
                local.get $11
                i32.le_u
                br_if $block_0
                local.get $1
                local.get $11
                i32.add
                local.tee $14
                i32.load8_s
                i32.const -65
                i32.le_s
                br_if $block_0
                local.get $14
                local.set $1
                local.get $2
                local.get $11
                i32.sub
                local.tee $2
                br_if $loop
                br $block_2
              end ;; $loop
            end ;; $block_3
            local.get $3
            i32.const 48
            i32.add
            global.set $12
            i32.const 1
            return
          end ;; $block_2
          local.get $3
          i32.const 48
          i32.add
          global.set $12
          i32.const 0
          return
        end ;; $block_1
        local.get $1
        local.get $2
        i32.const 0
        local.get $11
        call $core::str::slice_error_fail::h5395f932d3594937
        unreachable
      end ;; $block_0
      local.get $1
      local.get $2
      local.get $11
      local.get $2
      call $core::str::slice_error_fail::h5395f932d3594937
      unreachable
    end ;; $block
    local.get $3
    i32.const 48
    i32.add
    global.set $12
    i32.const 0
    )
  
  (func $std::panicking::rust_panic_with_hook::hfee0088857287f5e (type $5)
    (param $0 i32)
    (local $1 i32)
    (local $2 i32)
    i32.const 1
    local.set $1
    block $block
      block $block_0
        block $block_1
          block $block_2
            i32.const 0
            i32.load offset=1055592
            i32.const 1
            i32.ne
            br_if $block_2
            i32.const 0
            i32.const 0
            i32.load offset=1055596
            i32.const 1
            i32.add
            local.tee $1
            i32.store offset=1055596
            local.get $1
            i32.const 3
            i32.lt_u
            br_if $block_1
            br $block_0
          end ;; $block_2
          i32.const 0
          i64.const 4294967297
          i64.store offset=1055592
        end ;; $block_1
        i32.const 0
        i32.load offset=1055600
        local.tee $2
        i32.const -1
        i32.le_s
        br_if $block_0
        i32.const 0
        local.get $2
        i32.store offset=1055600
        local.get $1
        i32.const 2
        i32.lt_u
        br_if $block
      end ;; $block_0
      unreachable
      unreachable
    end ;; $block
    call $rust_panic
    unreachable
    )
  
  (func $rust_panic (type $4)
    unreachable
    unreachable
    )
  
  (func $std::panicking::continue_panic_fmt::h9d276daacc0ab278 (type $5)
    (param $0 i32)
    (local $1 i32)
    global.get $12
    i32.const 16
    i32.sub
    local.tee $1
    global.set $12
    local.get $0
    i32.load offset=8
    call $<core::option::Option<T>>::unwrap::h4d2370870cfbc354
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
    call $std::panicking::rust_panic_with_hook::hfee0088857287f5e
    unreachable
    )
  
  (func $<core::option::Option<T>>::unwrap::h4d2370870cfbc354 (type $6)
    (param $0 i32)
    (result i32)
    block $block
      local.get $0
      i32.eqz
      br_if $block
      local.get $0
      return
    end ;; $block
    i32.const 1053344
    call $core::panicking::panic::h13421bef49a1c627
    unreachable
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
        block $block_1
          block $block_2
            block $block_3
              block $block_4
                local.get $1
                i32.const 8
                i32.le_u
                br_if $block_4
                i32.const 0
                local.set $2
                i32.const -64
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
                call $dlmalloc::dlmalloc::Dlmalloc::malloc::hb5084b40b14605ec
                local.tee $0
                i32.eqz
                br_if $block
                local.get $0
                i32.const -8
                i32.add
                local.set $2
                local.get $1
                i32.const -1
                i32.add
                local.tee $4
                local.get $0
                i32.and
                i32.eqz
                br_if $block_3
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
                local.get $6
                i32.const 3
                i32.and
                i32.eqz
                br_if $block_2
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
                call $dlmalloc::dlmalloc::Dlmalloc::dispose_chunk::hf54af7e93e82e2ab
                local.get $1
                i32.load offset=4
                local.tee $0
                i32.const 3
                i32.and
                br_if $block_1
                br $block_0
              end ;; $block_4
              local.get $0
              call $dlmalloc::dlmalloc::Dlmalloc::malloc::hb5084b40b14605ec
              return
            end ;; $block_3
            local.get $2
            local.tee $1
            i32.load offset=4
            local.tee $0
            i32.const 3
            i32.and
            br_if $block_1
            br $block_0
          end ;; $block_2
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
          local.get $1
          i32.load offset=4
          local.tee $0
          i32.const 3
          i32.and
          i32.eqz
          br_if $block_0
        end ;; $block_1
        local.get $0
        i32.const -8
        i32.and
        local.tee $2
        local.get $3
        i32.const 16
        i32.add
        i32.le_u
        br_if $block_0
        local.get $1
        i32.const 4
        i32.add
        local.get $3
        local.get $0
        i32.const 1
        i32.and
        i32.or
        i32.const 2
        i32.or
        i32.store
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
        call $dlmalloc::dlmalloc::Dlmalloc::dispose_chunk::hf54af7e93e82e2ab
      end ;; $block_0
      local.get $1
      i32.const 8
      i32.add
      local.set $2
    end ;; $block
    local.get $2
    )
  
  (func $dlmalloc::dlmalloc::Dlmalloc::malloc::hb5084b40b14605ec (type $6)
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
    (local $11 i32)
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
                          block $block_10
                            block $block_11
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
                                                                block $block_29
                                                                  block $block_30
                                                                    block $block_31
                                                                      block $block_32
                                                                        block $block_33
                                                                          block $block_34
                                                                            block $block_35
                                                                              block $block_36
                                                                                block $block_37
                                                                                  local.get $0
                                                                                  i32.const 244
                                                                                  i32.gt_u
                                                                                  br_if $block_37
                                                                                  i32.const 0
                                                                                  i32.load offset=1055136
                                                                                  local.tee $1
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
                                                                                  local.tee $3
                                                                                  i32.const 31
                                                                                  i32.and
                                                                                  local.tee $4
                                                                                  i32.shr_u
                                                                                  local.tee $0
                                                                                  i32.const 3
                                                                                  i32.and
                                                                                  i32.eqz
                                                                                  br_if $block_36
                                                                                  local.get $0
                                                                                  i32.const -1
                                                                                  i32.xor
                                                                                  i32.const 1
                                                                                  i32.and
                                                                                  local.get $3
                                                                                  i32.add
                                                                                  local.tee $2
                                                                                  i32.const 3
                                                                                  i32.shl
                                                                                  local.tee $4
                                                                                  i32.const 1055152
                                                                                  i32.add
                                                                                  i32.load
                                                                                  local.tee $0
                                                                                  i32.const 8
                                                                                  i32.add
                                                                                  local.set $5
                                                                                  local.get $0
                                                                                  i32.load offset=8
                                                                                  local.tee $3
                                                                                  local.get $4
                                                                                  i32.const 1055144
                                                                                  i32.add
                                                                                  local.tee $4
                                                                                  i32.eq
                                                                                  br_if $block_35
                                                                                  local.get $3
                                                                                  local.get $4
                                                                                  i32.store offset=12
                                                                                  local.get $4
                                                                                  i32.const 8
                                                                                  i32.add
                                                                                  local.get $3
                                                                                  i32.store
                                                                                  br $block_34
                                                                                end ;; $block_37
                                                                                i32.const 0
                                                                                local.set $3
                                                                                local.get $0
                                                                                i32.const -64
                                                                                i32.ge_u
                                                                                br_if $block_8
                                                                                local.get $0
                                                                                i32.const 11
                                                                                i32.add
                                                                                local.tee $0
                                                                                i32.const -8
                                                                                i32.and
                                                                                local.set $2
                                                                                i32.const 0
                                                                                i32.load offset=1055140
                                                                                local.tee $6
                                                                                i32.eqz
                                                                                br_if $block_27
                                                                                i32.const 0
                                                                                local.set $7
                                                                                block $block_38
                                                                                  local.get $0
                                                                                  i32.const 8
                                                                                  i32.shr_u
                                                                                  local.tee $0
                                                                                  i32.eqz
                                                                                  br_if $block_38
                                                                                  i32.const 31
                                                                                  local.set $7
                                                                                  local.get $2
                                                                                  i32.const 16777215
                                                                                  i32.gt_u
                                                                                  br_if $block_38
                                                                                  local.get $2
                                                                                  i32.const 38
                                                                                  local.get $0
                                                                                  i32.clz
                                                                                  local.tee $0
                                                                                  i32.sub
                                                                                  i32.const 31
                                                                                  i32.and
                                                                                  i32.shr_u
                                                                                  i32.const 1
                                                                                  i32.and
                                                                                  i32.const 31
                                                                                  local.get $0
                                                                                  i32.sub
                                                                                  i32.const 1
                                                                                  i32.shl
                                                                                  i32.or
                                                                                  local.set $7
                                                                                end ;; $block_38
                                                                                i32.const 0
                                                                                local.get $2
                                                                                i32.sub
                                                                                local.set $3
                                                                                local.get $7
                                                                                i32.const 2
                                                                                i32.shl
                                                                                i32.const 1055408
                                                                                i32.add
                                                                                i32.load
                                                                                local.tee $0
                                                                                i32.eqz
                                                                                br_if $block_30
                                                                                i32.const 0
                                                                                local.set $4
                                                                                local.get $2
                                                                                i32.const 0
                                                                                i32.const 25
                                                                                local.get $7
                                                                                i32.const 1
                                                                                i32.shr_u
                                                                                i32.sub
                                                                                i32.const 31
                                                                                i32.and
                                                                                local.get $7
                                                                                i32.const 31
                                                                                i32.eq
                                                                                select
                                                                                i32.shl
                                                                                local.set $5
                                                                                i32.const 0
                                                                                local.set $1
                                                                                loop $loop
                                                                                  block $block_39
                                                                                    local.get $0
                                                                                    i32.load offset=4
                                                                                    i32.const -8
                                                                                    i32.and
                                                                                    local.tee $8
                                                                                    local.get $2
                                                                                    i32.lt_u
                                                                                    br_if $block_39
                                                                                    local.get $8
                                                                                    local.get $2
                                                                                    i32.sub
                                                                                    local.tee $8
                                                                                    local.get $3
                                                                                    i32.ge_u
                                                                                    br_if $block_39
                                                                                    local.get $8
                                                                                    local.set $3
                                                                                    local.get $0
                                                                                    local.set $1
                                                                                    local.get $8
                                                                                    i32.eqz
                                                                                    br_if $block_32
                                                                                  end ;; $block_39
                                                                                  local.get $0
                                                                                  i32.const 20
                                                                                  i32.add
                                                                                  i32.load
                                                                                  local.tee $8
                                                                                  local.get $4
                                                                                  local.get $8
                                                                                  local.get $0
                                                                                  local.get $5
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
                                                                                  local.get $4
                                                                                  local.get $8
                                                                                  select
                                                                                  local.set $4
                                                                                  local.get $5
                                                                                  i32.const 1
                                                                                  i32.shl
                                                                                  local.set $5
                                                                                  local.get $0
                                                                                  br_if $loop
                                                                                end ;; $loop
                                                                                local.get $4
                                                                                i32.eqz
                                                                                br_if $block_31
                                                                                local.get $4
                                                                                local.set $0
                                                                                br $block_29
                                                                              end ;; $block_36
                                                                              local.get $2
                                                                              i32.const 0
                                                                              i32.load offset=1055536
                                                                              i32.le_u
                                                                              br_if $block_27
                                                                              local.get $0
                                                                              i32.eqz
                                                                              br_if $block_33
                                                                              local.get $0
                                                                              local.get $4
                                                                              i32.shl
                                                                              i32.const 2
                                                                              local.get $4
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
                                                                              local.tee $3
                                                                              i32.const 3
                                                                              i32.shl
                                                                              local.tee $5
                                                                              i32.const 1055152
                                                                              i32.add
                                                                              i32.load
                                                                              local.tee $0
                                                                              i32.load offset=8
                                                                              local.tee $4
                                                                              local.get $5
                                                                              i32.const 1055144
                                                                              i32.add
                                                                              local.tee $5
                                                                              i32.eq
                                                                              br_if $block_25
                                                                              local.get $4
                                                                              local.get $5
                                                                              i32.store offset=12
                                                                              local.get $5
                                                                              i32.const 8
                                                                              i32.add
                                                                              local.get $4
                                                                              i32.store
                                                                              br $block_24
                                                                            end ;; $block_35
                                                                            i32.const 0
                                                                            local.get $1
                                                                            i32.const -2
                                                                            local.get $2
                                                                            i32.rotl
                                                                            i32.and
                                                                            i32.store offset=1055136
                                                                          end ;; $block_34
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
                                                                          local.get $5
                                                                          return
                                                                        end ;; $block_33
                                                                        i32.const 0
                                                                        i32.load offset=1055140
                                                                        local.tee $0
                                                                        i32.eqz
                                                                        br_if $block_27
                                                                        local.get $0
                                                                        i32.const 0
                                                                        local.get $0
                                                                        i32.sub
                                                                        i32.and
                                                                        i32.ctz
                                                                        i32.const 2
                                                                        i32.shl
                                                                        i32.const 1055408
                                                                        i32.add
                                                                        i32.load
                                                                        local.tee $1
                                                                        i32.load offset=4
                                                                        i32.const -8
                                                                        i32.and
                                                                        local.get $2
                                                                        i32.sub
                                                                        local.set $3
                                                                        local.get $1
                                                                        local.set $4
                                                                        local.get $1
                                                                        i32.load offset=16
                                                                        local.tee $0
                                                                        i32.eqz
                                                                        br_if $block_9
                                                                        i32.const 0
                                                                        local.set $9
                                                                        br $block
                                                                      end ;; $block_32
                                                                      i32.const 0
                                                                      local.set $3
                                                                      local.get $0
                                                                      local.set $1
                                                                      br $block_29
                                                                    end ;; $block_31
                                                                    local.get $1
                                                                    br_if $block_28
                                                                  end ;; $block_30
                                                                  i32.const 0
                                                                  local.set $1
                                                                  i32.const 2
                                                                  local.get $7
                                                                  i32.const 31
                                                                  i32.and
                                                                  i32.shl
                                                                  local.tee $0
                                                                  i32.const 0
                                                                  local.get $0
                                                                  i32.sub
                                                                  i32.or
                                                                  local.get $6
                                                                  i32.and
                                                                  local.tee $0
                                                                  i32.eqz
                                                                  br_if $block_27
                                                                  local.get $0
                                                                  i32.const 0
                                                                  local.get $0
                                                                  i32.sub
                                                                  i32.and
                                                                  i32.ctz
                                                                  i32.const 2
                                                                  i32.shl
                                                                  i32.const 1055408
                                                                  i32.add
                                                                  i32.load
                                                                  local.tee $0
                                                                  i32.eqz
                                                                  br_if $block_27
                                                                end ;; $block_29
                                                                block $block_40
                                                                  loop $loop_0
                                                                    local.get $0
                                                                    i32.load offset=4
                                                                    i32.const -8
                                                                    i32.and
                                                                    local.tee $4
                                                                    local.get $2
                                                                    i32.ge_u
                                                                    local.get $4
                                                                    local.get $2
                                                                    i32.sub
                                                                    local.tee $8
                                                                    local.get $3
                                                                    i32.lt_u
                                                                    i32.and
                                                                    local.set $4
                                                                    block $block_41
                                                                      local.get $0
                                                                      i32.load offset=16
                                                                      local.tee $5
                                                                      i32.eqz
                                                                      br_if $block_41
                                                                      local.get $0
                                                                      local.get $1
                                                                      local.get $4
                                                                      select
                                                                      local.set $1
                                                                      local.get $8
                                                                      local.get $3
                                                                      local.get $4
                                                                      select
                                                                      local.set $3
                                                                      local.get $5
                                                                      local.set $0
                                                                      local.get $5
                                                                      br_if $loop_0
                                                                      br $block_40
                                                                    end ;; $block_41
                                                                    local.get $0
                                                                    local.get $1
                                                                    local.get $4
                                                                    select
                                                                    local.set $1
                                                                    local.get $8
                                                                    local.get $3
                                                                    local.get $4
                                                                    select
                                                                    local.set $3
                                                                    local.get $0
                                                                    i32.const 20
                                                                    i32.add
                                                                    i32.load
                                                                    local.tee $4
                                                                    local.set $0
                                                                    local.get $4
                                                                    br_if $loop_0
                                                                  end ;; $loop_0
                                                                end ;; $block_40
                                                                local.get $1
                                                                i32.eqz
                                                                br_if $block_27
                                                              end ;; $block_28
                                                              i32.const 0
                                                              i32.load offset=1055536
                                                              local.tee $0
                                                              local.get $2
                                                              i32.lt_u
                                                              br_if $block_26
                                                              local.get $3
                                                              local.get $0
                                                              local.get $2
                                                              i32.sub
                                                              i32.lt_u
                                                              br_if $block_26
                                                            end ;; $block_27
                                                            block $block_42
                                                              block $block_43
                                                                block $block_44
                                                                  i32.const 0
                                                                  i32.load offset=1055536
                                                                  local.tee $0
                                                                  local.get $2
                                                                  i32.ge_u
                                                                  br_if $block_44
                                                                  i32.const 0
                                                                  i32.load offset=1055540
                                                                  local.tee $0
                                                                  local.get $2
                                                                  i32.le_u
                                                                  br_if $block_43
                                                                  i32.const 0
                                                                  local.get $0
                                                                  local.get $2
                                                                  i32.sub
                                                                  local.tee $3
                                                                  i32.store offset=1055540
                                                                  i32.const 0
                                                                  i32.const 0
                                                                  i32.load offset=1055548
                                                                  local.tee $0
                                                                  local.get $2
                                                                  i32.add
                                                                  local.tee $4
                                                                  i32.store offset=1055548
                                                                  local.get $4
                                                                  local.get $3
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
                                                                end ;; $block_44
                                                                i32.const 0
                                                                i32.load offset=1055544
                                                                local.set $3
                                                                local.get $0
                                                                local.get $2
                                                                i32.sub
                                                                local.tee $4
                                                                i32.const 16
                                                                i32.ge_u
                                                                br_if $block_42
                                                                i32.const 0
                                                                i32.const 0
                                                                i32.store offset=1055544
                                                                i32.const 0
                                                                i32.const 0
                                                                i32.store offset=1055536
                                                                local.get $3
                                                                local.get $0
                                                                i32.const 3
                                                                i32.or
                                                                i32.store offset=4
                                                                local.get $3
                                                                local.get $0
                                                                i32.add
                                                                local.tee $0
                                                                i32.const 4
                                                                i32.add
                                                                local.get $0
                                                                i32.load offset=4
                                                                i32.const 1
                                                                i32.or
                                                                i32.store
                                                                local.get $3
                                                                i32.const 8
                                                                i32.add
                                                                return
                                                              end ;; $block_43
                                                              i32.const 0
                                                              local.set $3
                                                              local.get $2
                                                              i32.const 65583
                                                              i32.add
                                                              local.tee $4
                                                              i32.const 16
                                                              i32.shr_u
                                                              memory.grow
                                                              local.tee $0
                                                              i32.const -1
                                                              i32.eq
                                                              br_if $block_5
                                                              local.get $0
                                                              i32.const 16
                                                              i32.shl
                                                              local.tee $1
                                                              i32.eqz
                                                              br_if $block_6
                                                              i32.const 0
                                                              i32.const 0
                                                              i32.load offset=1055552
                                                              local.get $4
                                                              i32.const -65536
                                                              i32.and
                                                              local.tee $8
                                                              i32.add
                                                              local.tee $0
                                                              i32.store offset=1055552
                                                              i32.const 0
                                                              i32.const 0
                                                              i32.load offset=1055556
                                                              local.tee $3
                                                              local.get $0
                                                              local.get $0
                                                              local.get $3
                                                              i32.lt_u
                                                              select
                                                              i32.store offset=1055556
                                                              i32.const 0
                                                              i32.load offset=1055548
                                                              local.tee $3
                                                              i32.eqz
                                                              br_if $block_19
                                                              i32.const 1055560
                                                              local.set $0
                                                              loop $loop_1
                                                                local.get $0
                                                                i32.load
                                                                local.tee $4
                                                                local.get $0
                                                                i32.load offset=4
                                                                local.tee $5
                                                                i32.add
                                                                local.get $1
                                                                i32.eq
                                                                br_if $block_18
                                                                local.get $0
                                                                i32.load offset=8
                                                                local.tee $0
                                                                br_if $loop_1
                                                                br $block_0
                                                              end ;; $loop_1
                                                            end ;; $block_42
                                                            i32.const 0
                                                            local.get $4
                                                            i32.store offset=1055536
                                                            i32.const 0
                                                            local.get $3
                                                            local.get $2
                                                            i32.add
                                                            local.tee $1
                                                            i32.store offset=1055544
                                                            local.get $1
                                                            local.get $4
                                                            i32.const 1
                                                            i32.or
                                                            i32.store offset=4
                                                            local.get $3
                                                            local.get $0
                                                            i32.add
                                                            local.get $4
                                                            i32.store
                                                            local.get $3
                                                            i32.const 4
                                                            i32.add
                                                            local.get $2
                                                            i32.const 3
                                                            i32.or
                                                            i32.store
                                                            local.get $3
                                                            i32.const 8
                                                            i32.add
                                                            return
                                                          end ;; $block_26
                                                          local.get $1
                                                          i32.load offset=24
                                                          local.set $7
                                                          local.get $1
                                                          i32.load offset=12
                                                          local.tee $4
                                                          local.get $1
                                                          i32.eq
                                                          br_if $block_23
                                                          local.get $1
                                                          i32.load offset=8
                                                          local.tee $0
                                                          local.get $4
                                                          i32.store offset=12
                                                          local.get $4
                                                          local.get $0
                                                          i32.store offset=8
                                                          local.get $7
                                                          br_if $block_16
                                                          br $block_13
                                                        end ;; $block_25
                                                        i32.const 0
                                                        local.get $1
                                                        i32.const -2
                                                        local.get $3
                                                        i32.rotl
                                                        i32.and
                                                        i32.store offset=1055136
                                                      end ;; $block_24
                                                      local.get $0
                                                      i32.const 8
                                                      i32.add
                                                      local.set $4
                                                      local.get $0
                                                      local.get $2
                                                      i32.const 3
                                                      i32.or
                                                      i32.store offset=4
                                                      local.get $0
                                                      local.get $2
                                                      i32.add
                                                      local.tee $1
                                                      local.get $3
                                                      i32.const 3
                                                      i32.shl
                                                      local.tee $3
                                                      local.get $2
                                                      i32.sub
                                                      local.tee $2
                                                      i32.const 1
                                                      i32.or
                                                      i32.store offset=4
                                                      local.get $0
                                                      local.get $3
                                                      i32.add
                                                      local.get $2
                                                      i32.store
                                                      i32.const 0
                                                      i32.load offset=1055536
                                                      local.tee $0
                                                      i32.eqz
                                                      br_if $block_20
                                                      local.get $0
                                                      i32.const 3
                                                      i32.shr_u
                                                      local.tee $5
                                                      i32.const 3
                                                      i32.shl
                                                      i32.const 1055144
                                                      i32.add
                                                      local.set $3
                                                      i32.const 0
                                                      i32.load offset=1055544
                                                      local.set $0
                                                      i32.const 0
                                                      i32.load offset=1055136
                                                      local.tee $8
                                                      i32.const 1
                                                      local.get $5
                                                      i32.const 31
                                                      i32.and
                                                      i32.shl
                                                      local.tee $5
                                                      i32.and
                                                      i32.eqz
                                                      br_if $block_22
                                                      local.get $3
                                                      i32.load offset=8
                                                      local.set $5
                                                      br $block_21
                                                    end ;; $block_23
                                                    local.get $1
                                                    i32.const 20
                                                    i32.const 16
                                                    local.get $1
                                                    i32.const 20
                                                    i32.add
                                                    local.tee $4
                                                    i32.load
                                                    local.tee $5
                                                    select
                                                    i32.add
                                                    i32.load
                                                    local.tee $0
                                                    i32.eqz
                                                    br_if $block_17
                                                    local.get $4
                                                    local.get $1
                                                    i32.const 16
                                                    i32.add
                                                    local.get $5
                                                    select
                                                    local.set $5
                                                    block $block_45
                                                      loop $loop_2
                                                        local.get $5
                                                        local.set $8
                                                        block $block_46
                                                          local.get $0
                                                          local.tee $4
                                                          i32.const 20
                                                          i32.add
                                                          local.tee $5
                                                          i32.load
                                                          local.tee $0
                                                          i32.eqz
                                                          br_if $block_46
                                                          local.get $0
                                                          br_if $loop_2
                                                          br $block_45
                                                        end ;; $block_46
                                                        local.get $4
                                                        i32.const 16
                                                        i32.add
                                                        local.set $5
                                                        local.get $4
                                                        i32.load offset=16
                                                        local.tee $0
                                                        br_if $loop_2
                                                      end ;; $loop_2
                                                    end ;; $block_45
                                                    local.get $8
                                                    i32.const 0
                                                    i32.store
                                                    local.get $7
                                                    br_if $block_16
                                                    br $block_13
                                                  end ;; $block_22
                                                  i32.const 0
                                                  local.get $8
                                                  local.get $5
                                                  i32.or
                                                  i32.store offset=1055136
                                                  local.get $3
                                                  local.set $5
                                                end ;; $block_21
                                                local.get $3
                                                i32.const 8
                                                i32.add
                                                local.get $0
                                                i32.store
                                                local.get $5
                                                local.get $0
                                                i32.store offset=12
                                                local.get $0
                                                local.get $3
                                                i32.store offset=12
                                                local.get $0
                                                local.get $5
                                                i32.store offset=8
                                              end ;; $block_20
                                              i32.const 0
                                              local.get $1
                                              i32.store offset=1055544
                                              i32.const 0
                                              local.get $2
                                              i32.store offset=1055536
                                              local.get $4
                                              return
                                            end ;; $block_19
                                            block $block_47
                                              block $block_48
                                                i32.const 0
                                                i32.load offset=1055580
                                                local.tee $0
                                                i32.eqz
                                                br_if $block_48
                                                local.get $0
                                                local.get $1
                                                i32.le_u
                                                br_if $block_47
                                              end ;; $block_48
                                              i32.const 0
                                              local.get $1
                                              i32.store offset=1055580
                                            end ;; $block_47
                                            i32.const 0
                                            i32.const 4095
                                            i32.store offset=1055584
                                            i32.const 0
                                            local.get $8
                                            i32.store offset=1055564
                                            i32.const 0
                                            local.get $1
                                            i32.store offset=1055560
                                            i32.const 0
                                            i32.const 1055144
                                            i32.store offset=1055156
                                            i32.const 0
                                            i32.const 1055152
                                            i32.store offset=1055164
                                            i32.const 0
                                            i32.const 1055144
                                            i32.store offset=1055152
                                            i32.const 0
                                            i32.const 1055160
                                            i32.store offset=1055172
                                            i32.const 0
                                            i32.const 1055152
                                            i32.store offset=1055160
                                            i32.const 0
                                            i32.const 1055168
                                            i32.store offset=1055180
                                            i32.const 0
                                            i32.const 1055160
                                            i32.store offset=1055168
                                            i32.const 0
                                            i32.const 1055176
                                            i32.store offset=1055188
                                            i32.const 0
                                            i32.const 1055168
                                            i32.store offset=1055176
                                            i32.const 0
                                            i32.const 1055184
                                            i32.store offset=1055196
                                            i32.const 0
                                            i32.const 1055176
                                            i32.store offset=1055184
                                            i32.const 0
                                            i32.const 1055192
                                            i32.store offset=1055204
                                            i32.const 0
                                            i32.const 1055184
                                            i32.store offset=1055192
                                            i32.const 0
                                            i32.const 1055200
                                            i32.store offset=1055212
                                            i32.const 0
                                            i32.const 1055192
                                            i32.store offset=1055200
                                            i32.const 0
                                            i32.const 0
                                            i32.store offset=1055572
                                            i32.const 0
                                            i32.const 1055208
                                            i32.store offset=1055220
                                            i32.const 0
                                            i32.const 1055200
                                            i32.store offset=1055208
                                            i32.const 0
                                            i32.const 1055216
                                            i32.store offset=1055228
                                            i32.const 0
                                            i32.const 1055208
                                            i32.store offset=1055216
                                            i32.const 0
                                            i32.const 1055224
                                            i32.store offset=1055236
                                            i32.const 0
                                            i32.const 1055216
                                            i32.store offset=1055224
                                            i32.const 0
                                            i32.const 1055232
                                            i32.store offset=1055244
                                            i32.const 0
                                            i32.const 1055224
                                            i32.store offset=1055232
                                            i32.const 0
                                            i32.const 1055240
                                            i32.store offset=1055252
                                            i32.const 0
                                            i32.const 1055232
                                            i32.store offset=1055240
                                            i32.const 0
                                            i32.const 1055248
                                            i32.store offset=1055260
                                            i32.const 0
                                            i32.const 1055240
                                            i32.store offset=1055248
                                            i32.const 0
                                            i32.const 1055256
                                            i32.store offset=1055268
                                            i32.const 0
                                            i32.const 1055248
                                            i32.store offset=1055256
                                            i32.const 0
                                            i32.const 1055264
                                            i32.store offset=1055276
                                            i32.const 0
                                            i32.const 1055256
                                            i32.store offset=1055264
                                            i32.const 0
                                            i32.const 1055272
                                            i32.store offset=1055284
                                            i32.const 0
                                            i32.const 1055264
                                            i32.store offset=1055272
                                            i32.const 0
                                            i32.const 1055280
                                            i32.store offset=1055292
                                            i32.const 0
                                            i32.const 1055272
                                            i32.store offset=1055280
                                            i32.const 0
                                            i32.const 1055288
                                            i32.store offset=1055300
                                            i32.const 0
                                            i32.const 1055280
                                            i32.store offset=1055288
                                            i32.const 0
                                            i32.const 1055296
                                            i32.store offset=1055308
                                            i32.const 0
                                            i32.const 1055288
                                            i32.store offset=1055296
                                            i32.const 0
                                            i32.const 1055304
                                            i32.store offset=1055316
                                            i32.const 0
                                            i32.const 1055296
                                            i32.store offset=1055304
                                            i32.const 0
                                            i32.const 1055312
                                            i32.store offset=1055324
                                            i32.const 0
                                            i32.const 1055304
                                            i32.store offset=1055312
                                            i32.const 0
                                            i32.const 1055320
                                            i32.store offset=1055332
                                            i32.const 0
                                            i32.const 1055312
                                            i32.store offset=1055320
                                            i32.const 0
                                            i32.const 1055328
                                            i32.store offset=1055340
                                            i32.const 0
                                            i32.const 1055320
                                            i32.store offset=1055328
                                            i32.const 0
                                            i32.const 1055336
                                            i32.store offset=1055348
                                            i32.const 0
                                            i32.const 1055328
                                            i32.store offset=1055336
                                            i32.const 0
                                            i32.const 1055344
                                            i32.store offset=1055356
                                            i32.const 0
                                            i32.const 1055336
                                            i32.store offset=1055344
                                            i32.const 0
                                            i32.const 1055352
                                            i32.store offset=1055364
                                            i32.const 0
                                            i32.const 1055344
                                            i32.store offset=1055352
                                            i32.const 0
                                            i32.const 1055360
                                            i32.store offset=1055372
                                            i32.const 0
                                            i32.const 1055352
                                            i32.store offset=1055360
                                            i32.const 0
                                            i32.const 1055368
                                            i32.store offset=1055380
                                            i32.const 0
                                            i32.const 1055360
                                            i32.store offset=1055368
                                            i32.const 0
                                            i32.const 1055376
                                            i32.store offset=1055388
                                            i32.const 0
                                            i32.const 1055368
                                            i32.store offset=1055376
                                            i32.const 0
                                            i32.const 1055384
                                            i32.store offset=1055396
                                            i32.const 0
                                            i32.const 1055376
                                            i32.store offset=1055384
                                            i32.const 0
                                            i32.const 1055392
                                            i32.store offset=1055404
                                            i32.const 0
                                            i32.const 1055384
                                            i32.store offset=1055392
                                            i32.const 0
                                            local.get $1
                                            i32.store offset=1055548
                                            i32.const 0
                                            i32.const 1055392
                                            i32.store offset=1055400
                                            i32.const 0
                                            local.get $8
                                            i32.const -40
                                            i32.add
                                            local.tee $0
                                            i32.store offset=1055540
                                            local.get $1
                                            local.get $0
                                            i32.const 1
                                            i32.or
                                            i32.store offset=4
                                            local.get $1
                                            local.get $0
                                            i32.add
                                            i32.const 40
                                            i32.store offset=4
                                            i32.const 0
                                            i32.const 2097152
                                            i32.store offset=1055576
                                            i32.const 0
                                            local.set $3
                                            i32.const 0
                                            i32.load offset=1055540
                                            local.tee $0
                                            local.get $2
                                            i32.gt_u
                                            br_if $block_2
                                            br $block_1
                                          end ;; $block_18
                                          local.get $0
                                          i32.load offset=12
                                          i32.eqz
                                          br_if $block_15
                                          br $block_0
                                        end ;; $block_17
                                        i32.const 0
                                        local.set $4
                                        local.get $7
                                        i32.eqz
                                        br_if $block_13
                                      end ;; $block_16
                                      block $block_49
                                        block $block_50
                                          local.get $1
                                          i32.load offset=28
                                          i32.const 2
                                          i32.shl
                                          i32.const 1055408
                                          i32.add
                                          local.tee $0
                                          i32.load
                                          local.get $1
                                          i32.eq
                                          br_if $block_50
                                          local.get $7
                                          i32.const 16
                                          i32.const 20
                                          local.get $7
                                          i32.load offset=16
                                          local.get $1
                                          i32.eq
                                          select
                                          i32.add
                                          local.get $4
                                          i32.store
                                          local.get $4
                                          br_if $block_49
                                          br $block_13
                                        end ;; $block_50
                                        local.get $0
                                        local.get $4
                                        i32.store
                                        local.get $4
                                        i32.eqz
                                        br_if $block_14
                                      end ;; $block_49
                                      local.get $4
                                      local.get $7
                                      i32.store offset=24
                                      block $block_51
                                        local.get $1
                                        i32.load offset=16
                                        local.tee $0
                                        i32.eqz
                                        br_if $block_51
                                        local.get $4
                                        local.get $0
                                        i32.store offset=16
                                        local.get $0
                                        local.get $4
                                        i32.store offset=24
                                      end ;; $block_51
                                      local.get $1
                                      i32.const 20
                                      i32.add
                                      i32.load
                                      local.tee $0
                                      i32.eqz
                                      br_if $block_13
                                      local.get $4
                                      i32.const 20
                                      i32.add
                                      local.get $0
                                      i32.store
                                      local.get $0
                                      local.get $4
                                      i32.store offset=24
                                      local.get $3
                                      i32.const 15
                                      i32.le_u
                                      br_if $block_12
                                      br $block_11
                                    end ;; $block_15
                                    local.get $1
                                    local.get $3
                                    i32.le_u
                                    br_if $block_0
                                    local.get $4
                                    local.get $3
                                    i32.gt_u
                                    br_if $block_0
                                    local.get $0
                                    i32.const 4
                                    i32.add
                                    local.get $5
                                    local.get $8
                                    i32.add
                                    i32.store
                                    i32.const 0
                                    i32.const 0
                                    i32.load offset=1055548
                                    local.tee $0
                                    i32.const 15
                                    i32.add
                                    i32.const -8
                                    i32.and
                                    local.tee $3
                                    i32.const -8
                                    i32.add
                                    local.tee $4
                                    i32.store offset=1055548
                                    i32.const 0
                                    i32.const 0
                                    i32.load offset=1055540
                                    local.get $8
                                    i32.add
                                    local.tee $1
                                    local.get $0
                                    i32.const 8
                                    i32.add
                                    local.get $3
                                    i32.sub
                                    i32.add
                                    local.tee $3
                                    i32.store offset=1055540
                                    local.get $4
                                    local.get $3
                                    i32.const 1
                                    i32.or
                                    i32.store offset=4
                                    local.get $0
                                    local.get $1
                                    i32.add
                                    i32.const 40
                                    i32.store offset=4
                                    i32.const 0
                                    i32.const 2097152
                                    i32.store offset=1055576
                                    i32.const 0
                                    local.set $3
                                    i32.const 0
                                    i32.load offset=1055540
                                    local.tee $0
                                    local.get $2
                                    i32.gt_u
                                    br_if $block_4
                                    br $block_3
                                  end ;; $block_14
                                  i32.const 0
                                  i32.const 0
                                  i32.load offset=1055140
                                  i32.const -2
                                  local.get $1
                                  i32.const 28
                                  i32.add
                                  i32.load
                                  i32.rotl
                                  i32.and
                                  i32.store offset=1055140
                                end ;; $block_13
                                local.get $3
                                i32.const 15
                                i32.gt_u
                                br_if $block_11
                              end ;; $block_12
                              local.get $1
                              local.get $3
                              local.get $2
                              i32.add
                              local.tee $0
                              i32.const 3
                              i32.or
                              i32.store offset=4
                              local.get $1
                              local.get $0
                              i32.add
                              local.tee $0
                              local.get $0
                              i32.load offset=4
                              i32.const 1
                              i32.or
                              i32.store offset=4
                              br $block_10
                            end ;; $block_11
                            local.get $1
                            local.get $2
                            i32.const 3
                            i32.or
                            i32.store offset=4
                            local.get $1
                            local.get $2
                            i32.add
                            local.tee $2
                            local.get $3
                            i32.const 1
                            i32.or
                            i32.store offset=4
                            local.get $2
                            local.get $3
                            i32.add
                            local.get $3
                            i32.store
                            block $block_52
                              block $block_53
                                block $block_54
                                  block $block_55
                                    block $block_56
                                      block $block_57
                                        block $block_58
                                          block $block_59
                                            local.get $3
                                            i32.const 255
                                            i32.gt_u
                                            br_if $block_59
                                            local.get $3
                                            i32.const 3
                                            i32.shr_u
                                            local.tee $3
                                            i32.const 3
                                            i32.shl
                                            i32.const 1055144
                                            i32.add
                                            local.set $0
                                            i32.const 0
                                            i32.load offset=1055136
                                            local.tee $4
                                            i32.const 1
                                            local.get $3
                                            i32.const 31
                                            i32.and
                                            i32.shl
                                            local.tee $3
                                            i32.and
                                            i32.eqz
                                            br_if $block_58
                                            local.get $0
                                            i32.const 8
                                            i32.add
                                            local.set $4
                                            local.get $0
                                            i32.load offset=8
                                            local.set $3
                                            br $block_57
                                          end ;; $block_59
                                          i32.const 0
                                          local.set $0
                                          block $block_60
                                            local.get $3
                                            i32.const 8
                                            i32.shr_u
                                            local.tee $4
                                            i32.eqz
                                            br_if $block_60
                                            i32.const 31
                                            local.set $0
                                            local.get $3
                                            i32.const 16777215
                                            i32.gt_u
                                            br_if $block_60
                                            local.get $3
                                            i32.const 38
                                            local.get $4
                                            i32.clz
                                            local.tee $0
                                            i32.sub
                                            i32.const 31
                                            i32.and
                                            i32.shr_u
                                            i32.const 1
                                            i32.and
                                            i32.const 31
                                            local.get $0
                                            i32.sub
                                            i32.const 1
                                            i32.shl
                                            i32.or
                                            local.set $0
                                          end ;; $block_60
                                          local.get $2
                                          i64.const 0
                                          i64.store offset=16 align=4
                                          local.get $2
                                          local.get $0
                                          i32.store offset=28
                                          local.get $0
                                          i32.const 2
                                          i32.shl
                                          i32.const 1055408
                                          i32.add
                                          local.set $4
                                          i32.const 0
                                          i32.load offset=1055140
                                          local.tee $5
                                          i32.const 1
                                          local.get $0
                                          i32.const 31
                                          i32.and
                                          i32.shl
                                          local.tee $8
                                          i32.and
                                          i32.eqz
                                          br_if $block_56
                                          local.get $4
                                          i32.load
                                          local.tee $5
                                          i32.load offset=4
                                          i32.const -8
                                          i32.and
                                          local.get $3
                                          i32.ne
                                          br_if $block_55
                                          local.get $5
                                          local.set $0
                                          br $block_54
                                        end ;; $block_58
                                        i32.const 0
                                        local.get $4
                                        local.get $3
                                        i32.or
                                        i32.store offset=1055136
                                        local.get $0
                                        i32.const 8
                                        i32.add
                                        local.set $4
                                        local.get $0
                                        local.set $3
                                      end ;; $block_57
                                      local.get $4
                                      local.get $2
                                      i32.store
                                      local.get $3
                                      local.get $2
                                      i32.store offset=12
                                      local.get $2
                                      local.get $0
                                      i32.store offset=12
                                      local.get $2
                                      local.get $3
                                      i32.store offset=8
                                      br $block_10
                                    end ;; $block_56
                                    i32.const 0
                                    local.get $5
                                    local.get $8
                                    i32.or
                                    i32.store offset=1055140
                                    local.get $4
                                    local.get $2
                                    i32.store
                                    local.get $2
                                    local.get $4
                                    i32.store offset=24
                                    br $block_52
                                  end ;; $block_55
                                  local.get $3
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
                                  local.set $4
                                  loop $loop_3
                                    local.get $5
                                    local.get $4
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
                                    br_if $block_53
                                    local.get $4
                                    i32.const 1
                                    i32.shl
                                    local.set $4
                                    local.get $0
                                    local.set $5
                                    local.get $0
                                    i32.load offset=4
                                    i32.const -8
                                    i32.and
                                    local.get $3
                                    i32.ne
                                    br_if $loop_3
                                  end ;; $loop_3
                                end ;; $block_54
                                local.get $0
                                i32.load offset=8
                                local.tee $3
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
                                local.get $3
                                i32.store offset=8
                                br $block_10
                              end ;; $block_53
                              local.get $8
                              local.get $2
                              i32.store
                              local.get $2
                              local.get $5
                              i32.store offset=24
                            end ;; $block_52
                            local.get $2
                            local.get $2
                            i32.store offset=12
                            local.get $2
                            local.get $2
                            i32.store offset=8
                          end ;; $block_10
                          local.get $1
                          i32.const 8
                          i32.add
                          local.set $3
                          br $block_7
                        end ;; $block_9
                        i32.const 1
                        local.set $9
                        br $block
                      end ;; $block_8
                      i32.const 42
                      local.set $9
                      br $block
                    end ;; $block_7
                    i32.const 42
                    local.set $9
                    br $block
                  end ;; $block_6
                  i32.const 42
                  local.set $9
                  br $block
                end ;; $block_5
                i32.const 42
                local.set $9
                br $block
              end ;; $block_4
              i32.const 43
              local.set $9
              br $block
            end ;; $block_3
            i32.const 42
            local.set $9
            br $block
          end ;; $block_2
          i32.const 43
          local.set $9
          br $block
        end ;; $block_1
        i32.const 42
        local.set $9
        br $block
      end ;; $block_0
      i32.const 0
      i32.const 0
      i32.load offset=1055580
      local.tee $0
      local.get $1
      local.get $0
      local.get $1
      i32.lt_u
      select
      i32.store offset=1055580
      local.get $1
      local.get $8
      i32.add
      local.set $4
      i32.const 1055560
      local.set $0
      block $block_61
        block $block_62
          block $block_63
            block $block_64
              loop $loop_4
                local.get $0
                i32.load
                local.get $4
                i32.eq
                br_if $block_64
                local.get $0
                i32.load offset=8
                local.tee $0
                br_if $loop_4
                br $block_63
              end ;; $loop_4
            end ;; $block_64
            local.get $0
            i32.load offset=12
            i32.eqz
            br_if $block_62
          end ;; $block_63
          i32.const 1055560
          local.set $0
          i32.const 1055560
          i32.load
          local.tee $4
          local.get $3
          i32.le_u
          br_if $block_61
          i32.const 28
          local.set $9
          br $block
        end ;; $block_62
        local.get $0
        local.get $1
        i32.store
        local.get $0
        local.get $0
        i32.load offset=4
        local.get $8
        i32.add
        i32.store offset=4
        local.get $1
        local.get $2
        i32.const 3
        i32.or
        i32.store offset=4
        local.get $1
        local.get $2
        i32.add
        local.set $0
        local.get $4
        local.get $1
        i32.sub
        local.get $2
        i32.sub
        local.set $2
        block $block_65
          block $block_66
            block $block_67
              block $block_68
                block $block_69
                  block $block_70
                    block $block_71
                      block $block_72
                        i32.const 0
                        i32.load offset=1055548
                        local.get $4
                        i32.eq
                        br_if $block_72
                        i32.const 0
                        i32.load offset=1055544
                        local.get $4
                        i32.eq
                        br_if $block_71
                        local.get $4
                        i32.load offset=4
                        local.tee $3
                        i32.const 3
                        i32.and
                        i32.const 1
                        i32.ne
                        br_if $block_65
                        local.get $3
                        i32.const -8
                        i32.and
                        local.tee $6
                        i32.const 255
                        i32.gt_u
                        br_if $block_70
                        local.get $4
                        i32.load offset=12
                        local.tee $5
                        local.get $4
                        i32.load offset=8
                        local.tee $8
                        i32.eq
                        br_if $block_69
                        local.get $8
                        local.get $5
                        i32.store offset=12
                        local.get $5
                        local.get $8
                        i32.store offset=8
                        br $block_66
                      end ;; $block_72
                      i32.const 0
                      local.get $0
                      i32.store offset=1055548
                      i32.const 0
                      i32.const 0
                      i32.load offset=1055540
                      local.get $2
                      i32.add
                      local.tee $2
                      i32.store offset=1055540
                      local.get $0
                      local.get $2
                      i32.const 1
                      i32.or
                      i32.store offset=4
                      local.get $1
                      i32.const 8
                      i32.add
                      return
                    end ;; $block_71
                    i32.const 0
                    local.get $0
                    i32.store offset=1055544
                    i32.const 0
                    i32.const 0
                    i32.load offset=1055536
                    local.get $2
                    i32.add
                    local.tee $2
                    i32.store offset=1055536
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
                    local.get $1
                    i32.const 8
                    i32.add
                    return
                  end ;; $block_70
                  local.get $4
                  i32.load offset=24
                  local.set $11
                  local.get $4
                  i32.load offset=12
                  local.tee $5
                  local.get $4
                  i32.eq
                  br_if $block_68
                  local.get $4
                  i32.load offset=8
                  local.tee $3
                  local.get $5
                  i32.store offset=12
                  local.get $5
                  local.get $3
                  i32.store offset=8
                  local.get $11
                  br_if $block_67
                  br $block_66
                end ;; $block_69
                i32.const 0
                i32.const 0
                i32.load offset=1055136
                i32.const -2
                local.get $3
                i32.const 3
                i32.shr_u
                i32.rotl
                i32.and
                i32.store offset=1055136
                br $block_66
              end ;; $block_68
              block $block_73
                local.get $4
                i32.const 20
                i32.const 16
                local.get $4
                i32.load offset=20
                local.tee $5
                select
                i32.add
                i32.load
                local.tee $3
                i32.eqz
                br_if $block_73
                local.get $4
                i32.const 20
                i32.add
                local.get $4
                i32.const 16
                i32.add
                local.get $5
                select
                local.set $8
                loop $loop_5
                  local.get $8
                  local.set $7
                  block $block_74
                    local.get $3
                    local.tee $5
                    i32.const 20
                    i32.add
                    local.tee $8
                    i32.load
                    local.tee $3
                    br_if $block_74
                    local.get $5
                    i32.const 16
                    i32.add
                    local.set $8
                    local.get $5
                    i32.load offset=16
                    local.set $3
                  end ;; $block_74
                  local.get $3
                  br_if $loop_5
                end ;; $loop_5
                local.get $7
                i32.const 0
                i32.store
                local.get $11
                br_if $block_67
                br $block_66
              end ;; $block_73
              i32.const 0
              local.set $5
              local.get $11
              i32.eqz
              br_if $block_66
            end ;; $block_67
            block $block_75
              block $block_76
                block $block_77
                  local.get $4
                  i32.load offset=28
                  i32.const 2
                  i32.shl
                  i32.const 1055408
                  i32.add
                  local.tee $3
                  i32.load
                  local.get $4
                  i32.eq
                  br_if $block_77
                  local.get $11
                  i32.const 16
                  i32.const 20
                  local.get $11
                  i32.load offset=16
                  local.get $4
                  i32.eq
                  select
                  i32.add
                  local.get $5
                  i32.store
                  local.get $5
                  br_if $block_76
                  br $block_66
                end ;; $block_77
                local.get $3
                local.get $5
                i32.store
                local.get $5
                i32.eqz
                br_if $block_75
              end ;; $block_76
              local.get $5
              local.get $11
              i32.store offset=24
              block $block_78
                local.get $4
                i32.load offset=16
                local.tee $3
                i32.eqz
                br_if $block_78
                local.get $5
                local.get $3
                i32.store offset=16
                local.get $3
                local.get $5
                i32.store offset=24
              end ;; $block_78
              local.get $4
              i32.load offset=20
              local.tee $3
              i32.eqz
              br_if $block_66
              local.get $5
              i32.const 20
              i32.add
              local.get $3
              i32.store
              local.get $3
              local.get $5
              i32.store offset=24
              br $block_66
            end ;; $block_75
            i32.const 0
            i32.const 0
            i32.load offset=1055140
            i32.const -2
            local.get $4
            i32.const 28
            i32.add
            i32.load
            i32.rotl
            i32.and
            i32.store offset=1055140
          end ;; $block_66
          local.get $6
          local.get $2
          i32.add
          local.set $2
          local.get $4
          local.get $6
          i32.add
          local.set $4
        end ;; $block_65
        local.get $4
        local.get $4
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
          block $block_80
            block $block_81
              block $block_82
                block $block_83
                  block $block_84
                    block $block_85
                      block $block_86
                        local.get $2
                        i32.const 255
                        i32.gt_u
                        br_if $block_86
                        local.get $2
                        i32.const 3
                        i32.shr_u
                        local.tee $3
                        i32.const 3
                        i32.shl
                        i32.const 1055144
                        i32.add
                        local.set $2
                        i32.const 0
                        i32.load offset=1055136
                        local.tee $4
                        i32.const 1
                        local.get $3
                        i32.const 31
                        i32.and
                        i32.shl
                        local.tee $3
                        i32.and
                        i32.eqz
                        br_if $block_85
                        local.get $2
                        i32.const 8
                        i32.add
                        local.set $4
                        local.get $2
                        i32.load offset=8
                        local.set $3
                        br $block_84
                      end ;; $block_86
                      i32.const 0
                      local.set $3
                      block $block_87
                        local.get $2
                        i32.const 8
                        i32.shr_u
                        local.tee $4
                        i32.eqz
                        br_if $block_87
                        i32.const 31
                        local.set $3
                        local.get $2
                        i32.const 16777215
                        i32.gt_u
                        br_if $block_87
                        local.get $2
                        i32.const 38
                        local.get $4
                        i32.clz
                        local.tee $3
                        i32.sub
                        i32.const 31
                        i32.and
                        i32.shr_u
                        i32.const 1
                        i32.and
                        i32.const 31
                        local.get $3
                        i32.sub
                        i32.const 1
                        i32.shl
                        i32.or
                        local.set $3
                      end ;; $block_87
                      local.get $0
                      i64.const 0
                      i64.store offset=16 align=4
                      local.get $0
                      local.get $3
                      i32.store offset=28
                      local.get $3
                      i32.const 2
                      i32.shl
                      i32.const 1055408
                      i32.add
                      local.set $4
                      i32.const 0
                      i32.load offset=1055140
                      local.tee $5
                      i32.const 1
                      local.get $3
                      i32.const 31
                      i32.and
                      i32.shl
                      local.tee $8
                      i32.and
                      i32.eqz
                      br_if $block_83
                      local.get $4
                      i32.load
                      local.tee $5
                      i32.load offset=4
                      i32.const -8
                      i32.and
                      local.get $2
                      i32.ne
                      br_if $block_82
                      local.get $5
                      local.set $3
                      br $block_81
                    end ;; $block_85
                    i32.const 0
                    local.get $4
                    local.get $3
                    i32.or
                    i32.store offset=1055136
                    local.get $2
                    i32.const 8
                    i32.add
                    local.set $4
                    local.get $2
                    local.set $3
                  end ;; $block_84
                  local.get $4
                  local.get $0
                  i32.store
                  local.get $3
                  local.get $0
                  i32.store offset=12
                  local.get $0
                  local.get $2
                  i32.store offset=12
                  local.get $0
                  local.get $3
                  i32.store offset=8
                  local.get $1
                  i32.const 8
                  i32.add
                  return
                end ;; $block_83
                i32.const 0
                local.get $5
                local.get $8
                i32.or
                i32.store offset=1055140
                local.get $4
                local.get $0
                i32.store
                local.get $0
                local.get $4
                i32.store offset=24
                br $block_79
              end ;; $block_82
              local.get $2
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
              local.set $4
              loop $loop_6
                local.get $5
                local.get $4
                i32.const 29
                i32.shr_u
                i32.const 4
                i32.and
                i32.add
                i32.const 16
                i32.add
                local.tee $8
                i32.load
                local.tee $3
                i32.eqz
                br_if $block_80
                local.get $4
                i32.const 1
                i32.shl
                local.set $4
                local.get $3
                local.set $5
                local.get $3
                i32.load offset=4
                i32.const -8
                i32.and
                local.get $2
                i32.ne
                br_if $loop_6
              end ;; $loop_6
            end ;; $block_81
            local.get $3
            i32.load offset=8
            local.tee $2
            local.get $0
            i32.store offset=12
            local.get $3
            local.get $0
            i32.store offset=8
            local.get $0
            i32.const 0
            i32.store offset=24
            local.get $0
            local.get $3
            i32.store offset=12
            local.get $0
            local.get $2
            i32.store offset=8
            local.get $1
            i32.const 8
            i32.add
            return
          end ;; $block_80
          local.get $8
          local.get $0
          i32.store
          local.get $0
          local.get $5
          i32.store offset=24
        end ;; $block_79
        local.get $0
        local.get $0
        i32.store offset=12
        local.get $0
        local.get $0
        i32.store offset=8
        local.get $1
        i32.const 8
        i32.add
        return
      end ;; $block_61
      i32.const 27
      local.set $9
    end ;; $block
    loop $loop_7 (result i32)
      block $block_88
        block $block_89
          block $block_90
            block $block_91
              block $block_92
                block $block_93
                  block $block_94
                    block $block_95
                      block $block_96
                        block $block_97
                          block $block_98
                            block $block_99
                              block $block_100
                                block $block_101
                                  block $block_102
                                    block $block_103
                                      block $block_104
                                        block $block_105
                                          block $block_106
                                            block $block_107
                                              block $block_108
                                                block $block_109
                                                  block $block_110
                                                    block $block_111
                                                      block $block_112
                                                        block $block_113
                                                          block $block_114
                                                            block $block_115
                                                              block $block_116
                                                                block $block_117
                                                                  block $block_118
                                                                    block $block_119
                                                                      block $block_120
                                                                        block $block_121
                                                                          block $block_122
                                                                            block $block_123
                                                                              block $block_124
                                                                                block $block_125
                                                                                  block $block_126
                                                                                    block $block_127
                                                                                      block $block_128
                                                                                        block $block_129
                                                                                          block $block_130
                                                                                            block $block_131
                                                                                              block $block_132
                                                                                                block $block_133
                                                                                                  block $block_134
                                                                                                    block $block_135
                                                                                                      block $block_136
                                                                                                        block $block_137
                                                                                                          block $block_138
                                                                                                            block $block_139
                                                                                                              block $block_140
                                                                                                                block $block_141
                                                                                                                  block $block_142
                                                                                                                    block $block_143
                                                                                                                      block $block_144
                                                                                                                        block $block_145
                                                                                                                          block $block_146
                                                                                                                            block $block_147
                                                                                                                              block $block_148
                                                                                                                                block $block_149
                                                                                                                                  block $block_150
                                                                                                                                    block $block_151
                                                                                                                                      block $block_152
                                                                                                                                        block $block_153
                                                                                                                                          block $block_154
                                                                                                                                            block $block_155
                                                                                                                                              block $block_156
                                                                                                                                                block $block_157
                                                                                                                                                  block $block_158
                                                                                                                                                    block $block_159
                                                                                                                                                      block $block_160
                                                                                                                                                        block $block_161
                                                                                                                                                          block $block_162
                                                                                                                                                            block $block_163
                                                                                                                                                              block $block_164
                                                                                                                                                                block $block_165
                                                                                                                                                                  block $block_166
                                                                                                                                                                    block $block_167
                                                                                                                                                                      block $block_168
                                                                                                                                                                        block $block_169
                                                                                                                                                                          block $block_170
                                                                                                                                                                            block $block_171
                                                                                                                                                                              block $block_172
                                                                                                                                                                                block $block_173
                                                                                                                                                                                  block $block_174
                                                                                                                                                                                    block $block_175
                                                                                                                                                                                      block $block_176
                                                                                                                                                                                        block $block_177
                                                                                                                                                                                          block $block_178
                                                                                                                                                                                            block $block_179
                                                                                                                                                                                              block $block_180
                                                                                                                                                                                                block $block_181
                                                                                                                                                                                                  block $block_182
                                                                                                                                                                                                    block $block_183
                                                                                                                                                                                                      block $block_184
                                                                                                                                                                                                        block $block_185
                                                                                                                                                                                                          local.get $9
                                                                                                                                                                                                          br_table
                                                                                                                                                                                                            $block_185 $block_184 $block_183 $block_182 $block_160 $block_159 $block_157 $block_156 $block_155 $block_154 $block_150 $block_149 $block_148 $block_146 $block_145 $block_147
                                                                                                                                                                                                            $block_151 $block_152 $block_158 $block_153 $block_173 $block_172 $block_171 $block_170 $block_168 $block_169 $block_161 $block_180 $block_181 $block_179 $block_178 $block_177
                                                                                                                                                                                                            $block_176 $block_167 $block_166 $block_165 $block_164 $block_163 $block_141 $block_140 $block_139 $block_138 $block_133 $block_134 $block_137 $block_136 $block_135 $block_162
                                                                                                                                                                                                            $block_142 $block_175 $block_174 $block_143 $block_144
                                                                                                                                                                                                            $block_144 ;; default
                                                                                                                                                                                                        end ;; $block_185
                                                                                                                                                                                                        local.get $0
                                                                                                                                                                                                        i32.load offset=4
                                                                                                                                                                                                        i32.const -8
                                                                                                                                                                                                        i32.and
                                                                                                                                                                                                        local.get $2
                                                                                                                                                                                                        i32.sub
                                                                                                                                                                                                        local.tee $1
                                                                                                                                                                                                        local.get $3
                                                                                                                                                                                                        local.get $1
                                                                                                                                                                                                        local.get $3
                                                                                                                                                                                                        i32.lt_u
                                                                                                                                                                                                        local.tee $1
                                                                                                                                                                                                        select
                                                                                                                                                                                                        local.set $3
                                                                                                                                                                                                        local.get $0
                                                                                                                                                                                                        local.get $4
                                                                                                                                                                                                        local.get $1
                                                                                                                                                                                                        select
                                                                                                                                                                                                        local.set $4
                                                                                                                                                                                                        local.get $0
                                                                                                                                                                                                        local.tee $1
                                                                                                                                                                                                        i32.load offset=16
                                                                                                                                                                                                        local.tee $0
                                                                                                                                                                                                        br_if $block_131
                                                                                                                                                                                                        i32.const 1
                                                                                                                                                                                                        local.set $9
                                                                                                                                                                                                        br $loop_7
                                                                                                                                                                                                      end ;; $block_184
                                                                                                                                                                                                      local.get $1
                                                                                                                                                                                                      i32.const 20
                                                                                                                                                                                                      i32.add
                                                                                                                                                                                                      i32.load
                                                                                                                                                                                                      local.tee $0
                                                                                                                                                                                                      br_if $block_130
                                                                                                                                                                                                      i32.const 2
                                                                                                                                                                                                      local.set $9
                                                                                                                                                                                                      br $loop_7
                                                                                                                                                                                                    end ;; $block_183
                                                                                                                                                                                                    local.get $4
                                                                                                                                                                                                    i32.load offset=24
                                                                                                                                                                                                    local.set $7
                                                                                                                                                                                                    local.get $4
                                                                                                                                                                                                    i32.load offset=12
                                                                                                                                                                                                    local.tee $1
                                                                                                                                                                                                    local.get $4
                                                                                                                                                                                                    i32.eq
                                                                                                                                                                                                    br_if $block_129
                                                                                                                                                                                                    i32.const 3
                                                                                                                                                                                                    local.set $9
                                                                                                                                                                                                    br $loop_7
                                                                                                                                                                                                  end ;; $block_182
                                                                                                                                                                                                  local.get $4
                                                                                                                                                                                                  i32.load offset=8
                                                                                                                                                                                                  local.tee $0
                                                                                                                                                                                                  local.get $1
                                                                                                                                                                                                  i32.store offset=12
                                                                                                                                                                                                  local.get $1
                                                                                                                                                                                                  local.get $0
                                                                                                                                                                                                  i32.store offset=8
                                                                                                                                                                                                  local.get $7
                                                                                                                                                                                                  br_if $block_127
                                                                                                                                                                                                  br $block_128
                                                                                                                                                                                                end ;; $block_181
                                                                                                                                                                                                local.get $0
                                                                                                                                                                                                i32.load offset=8
                                                                                                                                                                                                local.tee $0
                                                                                                                                                                                                i32.load
                                                                                                                                                                                                local.tee $4
                                                                                                                                                                                                local.get $3
                                                                                                                                                                                                i32.gt_u
                                                                                                                                                                                                br_if $block_106
                                                                                                                                                                                                i32.const 27
                                                                                                                                                                                                local.set $9
                                                                                                                                                                                                br $loop_7
                                                                                                                                                                                              end ;; $block_180
                                                                                                                                                                                              local.get $4
                                                                                                                                                                                              local.get $0
                                                                                                                                                                                              i32.load offset=4
                                                                                                                                                                                              i32.add
                                                                                                                                                                                              local.tee $4
                                                                                                                                                                                              local.get $3
                                                                                                                                                                                              i32.le_u
                                                                                                                                                                                              br_if $block_132
                                                                                                                                                                                              i32.const 29
                                                                                                                                                                                              local.set $9
                                                                                                                                                                                              br $loop_7
                                                                                                                                                                                            end ;; $block_179
                                                                                                                                                                                            i32.const 0
                                                                                                                                                                                            local.get $1
                                                                                                                                                                                            i32.store offset=1055548
                                                                                                                                                                                            i32.const 0
                                                                                                                                                                                            local.get $8
                                                                                                                                                                                            i32.const -40
                                                                                                                                                                                            i32.add
                                                                                                                                                                                            local.tee $0
                                                                                                                                                                                            i32.store offset=1055540
                                                                                                                                                                                            local.get $1
                                                                                                                                                                                            local.get $0
                                                                                                                                                                                            i32.const 1
                                                                                                                                                                                            i32.or
                                                                                                                                                                                            i32.store offset=4
                                                                                                                                                                                            local.get $1
                                                                                                                                                                                            local.get $0
                                                                                                                                                                                            i32.add
                                                                                                                                                                                            i32.const 40
                                                                                                                                                                                            i32.store offset=4
                                                                                                                                                                                            i32.const 0
                                                                                                                                                                                            i32.const 2097152
                                                                                                                                                                                            i32.store offset=1055576
                                                                                                                                                                                            local.get $3
                                                                                                                                                                                            local.get $4
                                                                                                                                                                                            i32.const -32
                                                                                                                                                                                            i32.add
                                                                                                                                                                                            i32.const -8
                                                                                                                                                                                            i32.and
                                                                                                                                                                                            i32.const -8
                                                                                                                                                                                            i32.add
                                                                                                                                                                                            local.tee $0
                                                                                                                                                                                            local.get $0
                                                                                                                                                                                            local.get $3
                                                                                                                                                                                            i32.const 16
                                                                                                                                                                                            i32.add
                                                                                                                                                                                            i32.lt_u
                                                                                                                                                                                            select
                                                                                                                                                                                            local.tee $5
                                                                                                                                                                                            i32.const 27
                                                                                                                                                                                            i32.store offset=4
                                                                                                                                                                                            i32.const 0
                                                                                                                                                                                            i64.load offset=1055560 align=4
                                                                                                                                                                                            local.set $10
                                                                                                                                                                                            local.get $5
                                                                                                                                                                                            i32.const 16
                                                                                                                                                                                            i32.add
                                                                                                                                                                                            i32.const 0
                                                                                                                                                                                            i64.load offset=1055568 align=4
                                                                                                                                                                                            i64.store align=4
                                                                                                                                                                                            local.get $5
                                                                                                                                                                                            local.get $10
                                                                                                                                                                                            i64.store offset=8 align=4
                                                                                                                                                                                            i32.const 0
                                                                                                                                                                                            local.get $8
                                                                                                                                                                                            i32.store offset=1055564
                                                                                                                                                                                            i32.const 0
                                                                                                                                                                                            local.get $1
                                                                                                                                                                                            i32.store offset=1055560
                                                                                                                                                                                            i32.const 0
                                                                                                                                                                                            local.get $5
                                                                                                                                                                                            i32.const 8
                                                                                                                                                                                            i32.add
                                                                                                                                                                                            i32.store offset=1055568
                                                                                                                                                                                            i32.const 0
                                                                                                                                                                                            i32.const 0
                                                                                                                                                                                            i32.store offset=1055572
                                                                                                                                                                                            local.get $5
                                                                                                                                                                                            i32.const 28
                                                                                                                                                                                            i32.add
                                                                                                                                                                                            local.set $0
                                                                                                                                                                                            i32.const 30
                                                                                                                                                                                            local.set $9
                                                                                                                                                                                            br $loop_7
                                                                                                                                                                                          end ;; $block_178
                                                                                                                                                                                          local.get $0
                                                                                                                                                                                          i32.const 7
                                                                                                                                                                                          i32.store
                                                                                                                                                                                          local.get $4
                                                                                                                                                                                          local.get $0
                                                                                                                                                                                          i32.const 4
                                                                                                                                                                                          i32.add
                                                                                                                                                                                          local.tee $0
                                                                                                                                                                                          i32.gt_u
                                                                                                                                                                                          br_if $block_105
                                                                                                                                                                                          i32.const 31
                                                                                                                                                                                          local.set $9
                                                                                                                                                                                          br $loop_7
                                                                                                                                                                                        end ;; $block_177
                                                                                                                                                                                        local.get $5
                                                                                                                                                                                        local.get $3
                                                                                                                                                                                        i32.eq
                                                                                                                                                                                        br_if $block_104
                                                                                                                                                                                        i32.const 32
                                                                                                                                                                                        local.set $9
                                                                                                                                                                                        br $loop_7
                                                                                                                                                                                      end ;; $block_176
                                                                                                                                                                                      local.get $5
                                                                                                                                                                                      local.get $5
                                                                                                                                                                                      i32.load offset=4
                                                                                                                                                                                      i32.const -2
                                                                                                                                                                                      i32.and
                                                                                                                                                                                      i32.store offset=4
                                                                                                                                                                                      local.get $3
                                                                                                                                                                                      local.get $5
                                                                                                                                                                                      local.get $3
                                                                                                                                                                                      i32.sub
                                                                                                                                                                                      local.tee $1
                                                                                                                                                                                      i32.const 1
                                                                                                                                                                                      i32.or
                                                                                                                                                                                      i32.store offset=4
                                                                                                                                                                                      local.get $5
                                                                                                                                                                                      local.get $1
                                                                                                                                                                                      i32.store
                                                                                                                                                                                      local.get $1
                                                                                                                                                                                      i32.const 255
                                                                                                                                                                                      i32.gt_u
                                                                                                                                                                                      br_if $block_103
                                                                                                                                                                                      i32.const 49
                                                                                                                                                                                      local.set $9
                                                                                                                                                                                      br $loop_7
                                                                                                                                                                                    end ;; $block_175
                                                                                                                                                                                    local.get $1
                                                                                                                                                                                    i32.const 3
                                                                                                                                                                                    i32.shr_u
                                                                                                                                                                                    local.tee $4
                                                                                                                                                                                    i32.const 3
                                                                                                                                                                                    i32.shl
                                                                                                                                                                                    i32.const 1055144
                                                                                                                                                                                    i32.add
                                                                                                                                                                                    local.set $0
                                                                                                                                                                                    i32.const 0
                                                                                                                                                                                    i32.load offset=1055136
                                                                                                                                                                                    local.tee $1
                                                                                                                                                                                    i32.const 1
                                                                                                                                                                                    local.get $4
                                                                                                                                                                                    i32.const 31
                                                                                                                                                                                    i32.and
                                                                                                                                                                                    i32.shl
                                                                                                                                                                                    local.tee $4
                                                                                                                                                                                    i32.and
                                                                                                                                                                                    i32.eqz
                                                                                                                                                                                    br_if $block_89
                                                                                                                                                                                    i32.const 50
                                                                                                                                                                                    local.set $9
                                                                                                                                                                                    br $loop_7
                                                                                                                                                                                  end ;; $block_174
                                                                                                                                                                                  local.get $0
                                                                                                                                                                                  i32.load offset=8
                                                                                                                                                                                  local.set $4
                                                                                                                                                                                  br $block_88
                                                                                                                                                                                end ;; $block_173
                                                                                                                                                                                local.get $4
                                                                                                                                                                                i32.const 20
                                                                                                                                                                                i32.const 16
                                                                                                                                                                                local.get $4
                                                                                                                                                                                i32.const 20
                                                                                                                                                                                i32.add
                                                                                                                                                                                local.tee $1
                                                                                                                                                                                i32.load
                                                                                                                                                                                local.tee $5
                                                                                                                                                                                select
                                                                                                                                                                                i32.add
                                                                                                                                                                                i32.load
                                                                                                                                                                                local.tee $0
                                                                                                                                                                                i32.eqz
                                                                                                                                                                                br_if $block_111
                                                                                                                                                                                i32.const 21
                                                                                                                                                                                local.set $9
                                                                                                                                                                                br $loop_7
                                                                                                                                                                              end ;; $block_172
                                                                                                                                                                              local.get $1
                                                                                                                                                                              local.get $4
                                                                                                                                                                              i32.const 16
                                                                                                                                                                              i32.add
                                                                                                                                                                              local.get $5
                                                                                                                                                                              select
                                                                                                                                                                              local.set $5
                                                                                                                                                                              i32.const 22
                                                                                                                                                                              local.set $9
                                                                                                                                                                              br $loop_7
                                                                                                                                                                            end ;; $block_171
                                                                                                                                                                            local.get $5
                                                                                                                                                                            local.set $8
                                                                                                                                                                            local.get $0
                                                                                                                                                                            local.tee $1
                                                                                                                                                                            i32.const 20
                                                                                                                                                                            i32.add
                                                                                                                                                                            local.tee $5
                                                                                                                                                                            i32.load
                                                                                                                                                                            local.tee $0
                                                                                                                                                                            i32.eqz
                                                                                                                                                                            br_if $block_107
                                                                                                                                                                            i32.const 23
                                                                                                                                                                            local.set $9
                                                                                                                                                                            br $loop_7
                                                                                                                                                                          end ;; $block_170
                                                                                                                                                                          local.get $0
                                                                                                                                                                          br_if $block_109
                                                                                                                                                                          br $block_110
                                                                                                                                                                        end ;; $block_169
                                                                                                                                                                        local.get $1
                                                                                                                                                                        i32.const 16
                                                                                                                                                                        i32.add
                                                                                                                                                                        local.set $5
                                                                                                                                                                        local.get $1
                                                                                                                                                                        i32.load offset=16
                                                                                                                                                                        local.tee $0
                                                                                                                                                                        br_if $block_108
                                                                                                                                                                        i32.const 24
                                                                                                                                                                        local.set $9
                                                                                                                                                                        br $loop_7
                                                                                                                                                                      end ;; $block_168
                                                                                                                                                                      local.get $8
                                                                                                                                                                      i32.const 0
                                                                                                                                                                      i32.store
                                                                                                                                                                      local.get $7
                                                                                                                                                                      br_if $block_125
                                                                                                                                                                      br $block_126
                                                                                                                                                                    end ;; $block_167
                                                                                                                                                                    i32.const 0
                                                                                                                                                                    local.set $0
                                                                                                                                                                    local.get $1
                                                                                                                                                                    i32.const 8
                                                                                                                                                                    i32.shr_u
                                                                                                                                                                    local.tee $4
                                                                                                                                                                    i32.eqz
                                                                                                                                                                    br_if $block_102
                                                                                                                                                                    i32.const 34
                                                                                                                                                                    local.set $9
                                                                                                                                                                    br $loop_7
                                                                                                                                                                  end ;; $block_166
                                                                                                                                                                  i32.const 31
                                                                                                                                                                  local.set $0
                                                                                                                                                                  local.get $1
                                                                                                                                                                  i32.const 16777215
                                                                                                                                                                  i32.gt_u
                                                                                                                                                                  br_if $block_101
                                                                                                                                                                  i32.const 35
                                                                                                                                                                  local.set $9
                                                                                                                                                                  br $loop_7
                                                                                                                                                                end ;; $block_165
                                                                                                                                                                local.get $1
                                                                                                                                                                i32.const 38
                                                                                                                                                                local.get $4
                                                                                                                                                                i32.clz
                                                                                                                                                                local.tee $0
                                                                                                                                                                i32.sub
                                                                                                                                                                i32.const 31
                                                                                                                                                                i32.and
                                                                                                                                                                i32.shr_u
                                                                                                                                                                i32.const 1
                                                                                                                                                                i32.and
                                                                                                                                                                i32.const 31
                                                                                                                                                                local.get $0
                                                                                                                                                                i32.sub
                                                                                                                                                                i32.const 1
                                                                                                                                                                i32.shl
                                                                                                                                                                i32.or
                                                                                                                                                                local.set $0
                                                                                                                                                                i32.const 36
                                                                                                                                                                local.set $9
                                                                                                                                                                br $loop_7
                                                                                                                                                              end ;; $block_164
                                                                                                                                                              local.get $3
                                                                                                                                                              i64.const 0
                                                                                                                                                              i64.store offset=16 align=4
                                                                                                                                                              local.get $3
                                                                                                                                                              i32.const 28
                                                                                                                                                              i32.add
                                                                                                                                                              local.get $0
                                                                                                                                                              i32.store
                                                                                                                                                              local.get $0
                                                                                                                                                              i32.const 2
                                                                                                                                                              i32.shl
                                                                                                                                                              i32.const 1055408
                                                                                                                                                              i32.add
                                                                                                                                                              local.set $4
                                                                                                                                                              i32.const 0
                                                                                                                                                              i32.load offset=1055140
                                                                                                                                                              local.tee $5
                                                                                                                                                              i32.const 1
                                                                                                                                                              local.get $0
                                                                                                                                                              i32.const 31
                                                                                                                                                              i32.and
                                                                                                                                                              i32.shl
                                                                                                                                                              local.tee $8
                                                                                                                                                              i32.and
                                                                                                                                                              i32.eqz
                                                                                                                                                              br_if $block_100
                                                                                                                                                              i32.const 37
                                                                                                                                                              local.set $9
                                                                                                                                                              br $loop_7
                                                                                                                                                            end ;; $block_163
                                                                                                                                                            local.get $4
                                                                                                                                                            i32.load
                                                                                                                                                            local.tee $5
                                                                                                                                                            i32.load offset=4
                                                                                                                                                            i32.const -8
                                                                                                                                                            i32.and
                                                                                                                                                            local.get $1
                                                                                                                                                            i32.ne
                                                                                                                                                            br_if $block_99
                                                                                                                                                            i32.const 47
                                                                                                                                                            local.set $9
                                                                                                                                                            br $loop_7
                                                                                                                                                          end ;; $block_162
                                                                                                                                                          local.get $5
                                                                                                                                                          local.set $0
                                                                                                                                                          br $block_96
                                                                                                                                                        end ;; $block_161
                                                                                                                                                        i32.const 0
                                                                                                                                                        local.set $1
                                                                                                                                                        local.get $7
                                                                                                                                                        i32.eqz
                                                                                                                                                        br_if $block_124
                                                                                                                                                        i32.const 4
                                                                                                                                                        local.set $9
                                                                                                                                                        br $loop_7
                                                                                                                                                      end ;; $block_160
                                                                                                                                                      local.get $4
                                                                                                                                                      i32.load offset=28
                                                                                                                                                      i32.const 2
                                                                                                                                                      i32.shl
                                                                                                                                                      i32.const 1055408
                                                                                                                                                      i32.add
                                                                                                                                                      local.tee $0
                                                                                                                                                      i32.load
                                                                                                                                                      local.get $4
                                                                                                                                                      i32.eq
                                                                                                                                                      br_if $block_123
                                                                                                                                                      i32.const 5
                                                                                                                                                      local.set $9
                                                                                                                                                      br $loop_7
                                                                                                                                                    end ;; $block_159
                                                                                                                                                    local.get $7
                                                                                                                                                    i32.const 16
                                                                                                                                                    i32.const 20
                                                                                                                                                    local.get $7
                                                                                                                                                    i32.load offset=16
                                                                                                                                                    local.get $4
                                                                                                                                                    i32.eq
                                                                                                                                                    select
                                                                                                                                                    i32.add
                                                                                                                                                    local.get $1
                                                                                                                                                    i32.store
                                                                                                                                                    local.get $1
                                                                                                                                                    br_if $block_121
                                                                                                                                                    br $block_122
                                                                                                                                                  end ;; $block_158
                                                                                                                                                  local.get $0
                                                                                                                                                  local.get $1
                                                                                                                                                  i32.store
                                                                                                                                                  local.get $1
                                                                                                                                                  i32.eqz
                                                                                                                                                  br_if $block_120
                                                                                                                                                  i32.const 6
                                                                                                                                                  local.set $9
                                                                                                                                                  br $loop_7
                                                                                                                                                end ;; $block_157
                                                                                                                                                local.get $1
                                                                                                                                                local.get $7
                                                                                                                                                i32.store offset=24
                                                                                                                                                local.get $4
                                                                                                                                                i32.load offset=16
                                                                                                                                                local.tee $0
                                                                                                                                                i32.eqz
                                                                                                                                                br_if $block_119
                                                                                                                                                i32.const 7
                                                                                                                                                local.set $9
                                                                                                                                                br $loop_7
                                                                                                                                              end ;; $block_156
                                                                                                                                              local.get $1
                                                                                                                                              local.get $0
                                                                                                                                              i32.store offset=16
                                                                                                                                              local.get $0
                                                                                                                                              local.get $1
                                                                                                                                              i32.store offset=24
                                                                                                                                              i32.const 8
                                                                                                                                              local.set $9
                                                                                                                                              br $loop_7
                                                                                                                                            end ;; $block_155
                                                                                                                                            local.get $4
                                                                                                                                            i32.const 20
                                                                                                                                            i32.add
                                                                                                                                            i32.load
                                                                                                                                            local.tee $0
                                                                                                                                            i32.eqz
                                                                                                                                            br_if $block_118
                                                                                                                                            i32.const 9
                                                                                                                                            local.set $9
                                                                                                                                            br $loop_7
                                                                                                                                          end ;; $block_154
                                                                                                                                          local.get $1
                                                                                                                                          i32.const 20
                                                                                                                                          i32.add
                                                                                                                                          local.get $0
                                                                                                                                          i32.store
                                                                                                                                          local.get $0
                                                                                                                                          local.get $1
                                                                                                                                          i32.store offset=24
                                                                                                                                          local.get $3
                                                                                                                                          i32.const 16
                                                                                                                                          i32.lt_u
                                                                                                                                          br_if $block_116
                                                                                                                                          br $block_115
                                                                                                                                        end ;; $block_153
                                                                                                                                        i32.const 0
                                                                                                                                        i32.const 0
                                                                                                                                        i32.load offset=1055140
                                                                                                                                        i32.const -2
                                                                                                                                        local.get $4
                                                                                                                                        i32.const 28
                                                                                                                                        i32.add
                                                                                                                                        i32.load
                                                                                                                                        i32.rotl
                                                                                                                                        i32.and
                                                                                                                                        i32.store offset=1055140
                                                                                                                                        i32.const 17
                                                                                                                                        local.set $9
                                                                                                                                        br $loop_7
                                                                                                                                      end ;; $block_152
                                                                                                                                      local.get $3
                                                                                                                                      i32.const 16
                                                                                                                                      i32.ge_u
                                                                                                                                      br_if $block_117
                                                                                                                                      i32.const 16
                                                                                                                                      local.set $9
                                                                                                                                      br $loop_7
                                                                                                                                    end ;; $block_151
                                                                                                                                    local.get $4
                                                                                                                                    local.get $3
                                                                                                                                    local.get $2
                                                                                                                                    i32.add
                                                                                                                                    local.tee $0
                                                                                                                                    i32.const 3
                                                                                                                                    i32.or
                                                                                                                                    i32.store offset=4
                                                                                                                                    local.get $4
                                                                                                                                    local.get $0
                                                                                                                                    i32.add
                                                                                                                                    local.tee $0
                                                                                                                                    local.get $0
                                                                                                                                    i32.load offset=4
                                                                                                                                    i32.const 1
                                                                                                                                    i32.or
                                                                                                                                    i32.store offset=4
                                                                                                                                    local.get $4
                                                                                                                                    i32.const 8
                                                                                                                                    i32.add
                                                                                                                                    return
                                                                                                                                  end ;; $block_150
                                                                                                                                  local.get $4
                                                                                                                                  local.get $2
                                                                                                                                  i32.const 3
                                                                                                                                  i32.or
                                                                                                                                  i32.store offset=4
                                                                                                                                  local.get $4
                                                                                                                                  local.get $2
                                                                                                                                  i32.add
                                                                                                                                  local.tee $2
                                                                                                                                  local.get $3
                                                                                                                                  i32.const 1
                                                                                                                                  i32.or
                                                                                                                                  i32.store offset=4
                                                                                                                                  local.get $2
                                                                                                                                  local.get $3
                                                                                                                                  i32.add
                                                                                                                                  local.get $3
                                                                                                                                  i32.store
                                                                                                                                  i32.const 0
                                                                                                                                  i32.load offset=1055536
                                                                                                                                  local.tee $0
                                                                                                                                  i32.eqz
                                                                                                                                  br_if $block_114
                                                                                                                                  i32.const 11
                                                                                                                                  local.set $9
                                                                                                                                  br $loop_7
                                                                                                                                end ;; $block_149
                                                                                                                                local.get $0
                                                                                                                                i32.const 3
                                                                                                                                i32.shr_u
                                                                                                                                local.tee $5
                                                                                                                                i32.const 3
                                                                                                                                i32.shl
                                                                                                                                i32.const 1055144
                                                                                                                                i32.add
                                                                                                                                local.set $1
                                                                                                                                i32.const 0
                                                                                                                                i32.load offset=1055544
                                                                                                                                local.set $0
                                                                                                                                i32.const 0
                                                                                                                                i32.load offset=1055136
                                                                                                                                local.tee $8
                                                                                                                                i32.const 1
                                                                                                                                local.get $5
                                                                                                                                i32.const 31
                                                                                                                                i32.and
                                                                                                                                i32.shl
                                                                                                                                local.tee $5
                                                                                                                                i32.and
                                                                                                                                i32.eqz
                                                                                                                                br_if $block_113
                                                                                                                                i32.const 12
                                                                                                                                local.set $9
                                                                                                                                br $loop_7
                                                                                                                              end ;; $block_148
                                                                                                                              local.get $1
                                                                                                                              i32.load offset=8
                                                                                                                              local.set $5
                                                                                                                              br $block_112
                                                                                                                            end ;; $block_147
                                                                                                                            i32.const 0
                                                                                                                            local.get $8
                                                                                                                            local.get $5
                                                                                                                            i32.or
                                                                                                                            i32.store offset=1055136
                                                                                                                            local.get $1
                                                                                                                            local.set $5
                                                                                                                            i32.const 13
                                                                                                                            local.set $9
                                                                                                                            br $loop_7
                                                                                                                          end ;; $block_146
                                                                                                                          local.get $1
                                                                                                                          i32.const 8
                                                                                                                          i32.add
                                                                                                                          local.get $0
                                                                                                                          i32.store
                                                                                                                          local.get $5
                                                                                                                          local.get $0
                                                                                                                          i32.store offset=12
                                                                                                                          local.get $0
                                                                                                                          local.get $1
                                                                                                                          i32.store offset=12
                                                                                                                          local.get $0
                                                                                                                          local.get $5
                                                                                                                          i32.store offset=8
                                                                                                                          i32.const 14
                                                                                                                          local.set $9
                                                                                                                          br $loop_7
                                                                                                                        end ;; $block_145
                                                                                                                        i32.const 0
                                                                                                                        local.get $2
                                                                                                                        i32.store offset=1055544
                                                                                                                        i32.const 0
                                                                                                                        local.get $3
                                                                                                                        i32.store offset=1055536
                                                                                                                        local.get $4
                                                                                                                        i32.const 8
                                                                                                                        i32.add
                                                                                                                        return
                                                                                                                      end ;; $block_144
                                                                                                                      i32.const 0
                                                                                                                      local.get $1
                                                                                                                      local.get $4
                                                                                                                      i32.or
                                                                                                                      i32.store offset=1055136
                                                                                                                      local.get $0
                                                                                                                      local.set $4
                                                                                                                      i32.const 51
                                                                                                                      local.set $9
                                                                                                                      br $loop_7
                                                                                                                    end ;; $block_143
                                                                                                                    local.get $0
                                                                                                                    i32.const 8
                                                                                                                    i32.add
                                                                                                                    local.get $3
                                                                                                                    i32.store
                                                                                                                    local.get $4
                                                                                                                    local.get $3
                                                                                                                    i32.store offset=12
                                                                                                                    local.get $3
                                                                                                                    local.get $0
                                                                                                                    i32.store offset=12
                                                                                                                    local.get $3
                                                                                                                    local.get $4
                                                                                                                    i32.store offset=8
                                                                                                                    i32.const 0
                                                                                                                    local.set $3
                                                                                                                    i32.const 0
                                                                                                                    i32.load offset=1055540
                                                                                                                    local.tee $0
                                                                                                                    local.get $2
                                                                                                                    i32.gt_u
                                                                                                                    br_if $block_92
                                                                                                                    br $block_91
                                                                                                                  end ;; $block_142
                                                                                                                  i32.const 0
                                                                                                                  local.get $5
                                                                                                                  local.get $8
                                                                                                                  i32.or
                                                                                                                  i32.store offset=1055140
                                                                                                                  local.get $4
                                                                                                                  local.get $3
                                                                                                                  i32.store
                                                                                                                  local.get $3
                                                                                                                  i32.const 24
                                                                                                                  i32.add
                                                                                                                  local.get $4
                                                                                                                  i32.store
                                                                                                                  br $block_90
                                                                                                                end ;; $block_141
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
                                                                                                                local.set $4
                                                                                                                i32.const 39
                                                                                                                local.set $9
                                                                                                                br $loop_7
                                                                                                              end ;; $block_140
                                                                                                              local.get $5
                                                                                                              local.get $4
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
                                                                                                              br_if $block_97
                                                                                                              i32.const 40
                                                                                                              local.set $9
                                                                                                              br $loop_7
                                                                                                            end ;; $block_139
                                                                                                            local.get $4
                                                                                                            i32.const 1
                                                                                                            i32.shl
                                                                                                            local.set $4
                                                                                                            local.get $0
                                                                                                            local.set $5
                                                                                                            local.get $0
                                                                                                            i32.load offset=4
                                                                                                            i32.const -8
                                                                                                            i32.and
                                                                                                            local.get $1
                                                                                                            i32.ne
                                                                                                            br_if $block_98
                                                                                                            i32.const 41
                                                                                                            local.set $9
                                                                                                            br $loop_7
                                                                                                          end ;; $block_138
                                                                                                          local.get $0
                                                                                                          i32.load offset=8
                                                                                                          local.tee $4
                                                                                                          local.get $3
                                                                                                          i32.store offset=12
                                                                                                          local.get $0
                                                                                                          local.get $3
                                                                                                          i32.store offset=8
                                                                                                          local.get $3
                                                                                                          i32.const 24
                                                                                                          i32.add
                                                                                                          i32.const 0
                                                                                                          i32.store
                                                                                                          local.get $3
                                                                                                          local.get $0
                                                                                                          i32.store offset=12
                                                                                                          local.get $3
                                                                                                          local.get $4
                                                                                                          i32.store offset=8
                                                                                                          i32.const 0
                                                                                                          local.set $3
                                                                                                          i32.const 0
                                                                                                          i32.load offset=1055540
                                                                                                          local.tee $0
                                                                                                          local.get $2
                                                                                                          i32.gt_u
                                                                                                          br_if $block_94
                                                                                                          br $block_93
                                                                                                        end ;; $block_137
                                                                                                        local.get $8
                                                                                                        local.get $3
                                                                                                        i32.store
                                                                                                        local.get $3
                                                                                                        i32.const 24
                                                                                                        i32.add
                                                                                                        local.get $5
                                                                                                        i32.store
                                                                                                        i32.const 45
                                                                                                        local.set $9
                                                                                                        br $loop_7
                                                                                                      end ;; $block_136
                                                                                                      local.get $3
                                                                                                      local.get $3
                                                                                                      i32.store offset=12
                                                                                                      local.get $3
                                                                                                      local.get $3
                                                                                                      i32.store offset=8
                                                                                                      i32.const 46
                                                                                                      local.set $9
                                                                                                      br $loop_7
                                                                                                    end ;; $block_135
                                                                                                    i32.const 0
                                                                                                    local.set $3
                                                                                                    i32.const 0
                                                                                                    i32.load offset=1055540
                                                                                                    local.tee $0
                                                                                                    local.get $2
                                                                                                    i32.le_u
                                                                                                    br_if $block_95
                                                                                                    i32.const 43
                                                                                                    local.set $9
                                                                                                    br $loop_7
                                                                                                  end ;; $block_134
                                                                                                  i32.const 0
                                                                                                  local.get $0
                                                                                                  local.get $2
                                                                                                  i32.sub
                                                                                                  local.tee $3
                                                                                                  i32.store offset=1055540
                                                                                                  i32.const 0
                                                                                                  i32.const 0
                                                                                                  i32.load offset=1055548
                                                                                                  local.tee $0
                                                                                                  local.get $2
                                                                                                  i32.add
                                                                                                  local.tee $4
                                                                                                  i32.store offset=1055548
                                                                                                  local.get $4
                                                                                                  local.get $3
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
                                                                                                end ;; $block_133
                                                                                                local.get $3
                                                                                                return
                                                                                              end ;; $block_132
                                                                                              i32.const 28
                                                                                              local.set $9
                                                                                              br $loop_7
                                                                                            end ;; $block_131
                                                                                            i32.const 0
                                                                                            local.set $9
                                                                                            br $loop_7
                                                                                          end ;; $block_130
                                                                                          i32.const 0
                                                                                          local.set $9
                                                                                          br $loop_7
                                                                                        end ;; $block_129
                                                                                        i32.const 20
                                                                                        local.set $9
                                                                                        br $loop_7
                                                                                      end ;; $block_128
                                                                                      i32.const 17
                                                                                      local.set $9
                                                                                      br $loop_7
                                                                                    end ;; $block_127
                                                                                    i32.const 4
                                                                                    local.set $9
                                                                                    br $loop_7
                                                                                  end ;; $block_126
                                                                                  i32.const 17
                                                                                  local.set $9
                                                                                  br $loop_7
                                                                                end ;; $block_125
                                                                                i32.const 4
                                                                                local.set $9
                                                                                br $loop_7
                                                                              end ;; $block_124
                                                                              i32.const 17
                                                                              local.set $9
                                                                              br $loop_7
                                                                            end ;; $block_123
                                                                            i32.const 18
                                                                            local.set $9
                                                                            br $loop_7
                                                                          end ;; $block_122
                                                                          i32.const 17
                                                                          local.set $9
                                                                          br $loop_7
                                                                        end ;; $block_121
                                                                        i32.const 6
                                                                        local.set $9
                                                                        br $loop_7
                                                                      end ;; $block_120
                                                                      i32.const 19
                                                                      local.set $9
                                                                      br $loop_7
                                                                    end ;; $block_119
                                                                    i32.const 8
                                                                    local.set $9
                                                                    br $loop_7
                                                                  end ;; $block_118
                                                                  i32.const 17
                                                                  local.set $9
                                                                  br $loop_7
                                                                end ;; $block_117
                                                                i32.const 10
                                                                local.set $9
                                                                br $loop_7
                                                              end ;; $block_116
                                                              i32.const 16
                                                              local.set $9
                                                              br $loop_7
                                                            end ;; $block_115
                                                            i32.const 10
                                                            local.set $9
                                                            br $loop_7
                                                          end ;; $block_114
                                                          i32.const 14
                                                          local.set $9
                                                          br $loop_7
                                                        end ;; $block_113
                                                        i32.const 15
                                                        local.set $9
                                                        br $loop_7
                                                      end ;; $block_112
                                                      i32.const 13
                                                      local.set $9
                                                      br $loop_7
                                                    end ;; $block_111
                                                    i32.const 26
                                                    local.set $9
                                                    br $loop_7
                                                  end ;; $block_110
                                                  i32.const 24
                                                  local.set $9
                                                  br $loop_7
                                                end ;; $block_109
                                                i32.const 22
                                                local.set $9
                                                br $loop_7
                                              end ;; $block_108
                                              i32.const 22
                                              local.set $9
                                              br $loop_7
                                            end ;; $block_107
                                            i32.const 25
                                            local.set $9
                                            br $loop_7
                                          end ;; $block_106
                                          i32.const 28
                                          local.set $9
                                          br $loop_7
                                        end ;; $block_105
                                        i32.const 30
                                        local.set $9
                                        br $loop_7
                                      end ;; $block_104
                                      i32.const 46
                                      local.set $9
                                      br $loop_7
                                    end ;; $block_103
                                    i32.const 33
                                    local.set $9
                                    br $loop_7
                                  end ;; $block_102
                                  i32.const 36
                                  local.set $9
                                  br $loop_7
                                end ;; $block_101
                                i32.const 36
                                local.set $9
                                br $loop_7
                              end ;; $block_100
                              i32.const 48
                              local.set $9
                              br $loop_7
                            end ;; $block_99
                            i32.const 38
                            local.set $9
                            br $loop_7
                          end ;; $block_98
                          i32.const 39
                          local.set $9
                          br $loop_7
                        end ;; $block_97
                        i32.const 44
                        local.set $9
                        br $loop_7
                      end ;; $block_96
                      i32.const 41
                      local.set $9
                      br $loop_7
                    end ;; $block_95
                    i32.const 42
                    local.set $9
                    br $loop_7
                  end ;; $block_94
                  i32.const 43
                  local.set $9
                  br $loop_7
                end ;; $block_93
                i32.const 42
                local.set $9
                br $loop_7
              end ;; $block_92
              i32.const 43
              local.set $9
              br $loop_7
            end ;; $block_91
            i32.const 42
            local.set $9
            br $loop_7
          end ;; $block_90
          i32.const 45
          local.set $9
          br $loop_7
        end ;; $block_89
        i32.const 52
        local.set $9
        br $loop_7
      end ;; $block_88
      i32.const 51
      local.set $9
      br $loop_7
    end ;; $loop_7
    )
  
  (func $dlmalloc::dlmalloc::Dlmalloc::dispose_chunk::hf54af7e93e82e2ab (type $0)
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
              block $block_4
                block $block_5
                  block $block_6
                    block $block_7
                      block $block_8
                        block $block_9
                          block $block_10
                            block $block_11
                              local.get $0
                              i32.load offset=4
                              local.tee $3
                              i32.const 1
                              i32.and
                              br_if $block_11
                              local.get $3
                              i32.const 3
                              i32.and
                              i32.eqz
                              br_if $block_7
                              local.get $0
                              i32.load
                              local.tee $3
                              local.get $1
                              i32.add
                              local.set $1
                              block $block_12
                                block $block_13
                                  block $block_14
                                    block $block_15
                                      block $block_16
                                        i32.const 0
                                        i32.load offset=1055544
                                        local.get $0
                                        local.get $3
                                        i32.sub
                                        local.tee $0
                                        i32.eq
                                        br_if $block_16
                                        local.get $3
                                        i32.const 255
                                        i32.gt_u
                                        br_if $block_15
                                        local.get $0
                                        i32.load offset=12
                                        local.tee $4
                                        local.get $0
                                        i32.load offset=8
                                        local.tee $5
                                        i32.eq
                                        br_if $block_14
                                        local.get $5
                                        local.get $4
                                        i32.store offset=12
                                        local.get $4
                                        local.get $5
                                        i32.store offset=8
                                        local.get $2
                                        i32.load offset=4
                                        local.tee $3
                                        i32.const 2
                                        i32.and
                                        i32.eqz
                                        br_if $block_8
                                        br $block_9
                                      end ;; $block_16
                                      local.get $2
                                      i32.load offset=4
                                      i32.const 3
                                      i32.and
                                      i32.const 3
                                      i32.ne
                                      br_if $block_11
                                      i32.const 0
                                      local.get $1
                                      i32.store offset=1055536
                                      local.get $2
                                      i32.const 4
                                      i32.add
                                      local.tee $3
                                      local.get $3
                                      i32.load
                                      i32.const -2
                                      i32.and
                                      i32.store
                                      local.get $0
                                      local.get $1
                                      i32.const 1
                                      i32.or
                                      i32.store offset=4
                                      local.get $2
                                      local.get $1
                                      i32.store
                                      return
                                    end ;; $block_15
                                    local.get $0
                                    i32.load offset=24
                                    local.set $6
                                    local.get $0
                                    i32.load offset=12
                                    local.tee $4
                                    local.get $0
                                    i32.eq
                                    br_if $block_13
                                    local.get $0
                                    i32.load offset=8
                                    local.tee $3
                                    local.get $4
                                    i32.store offset=12
                                    local.get $4
                                    local.get $3
                                    i32.store offset=8
                                    local.get $6
                                    br_if $block_12
                                    br $block_11
                                  end ;; $block_14
                                  i32.const 0
                                  i32.const 0
                                  i32.load offset=1055136
                                  i32.const -2
                                  local.get $3
                                  i32.const 3
                                  i32.shr_u
                                  i32.rotl
                                  i32.and
                                  i32.store offset=1055136
                                  local.get $2
                                  i32.load offset=4
                                  local.tee $3
                                  i32.const 2
                                  i32.and
                                  i32.eqz
                                  br_if $block_8
                                  br $block_9
                                end ;; $block_13
                                block $block_17
                                  local.get $0
                                  i32.const 20
                                  i32.const 16
                                  local.get $0
                                  i32.load offset=20
                                  local.tee $4
                                  select
                                  i32.add
                                  i32.load
                                  local.tee $3
                                  i32.eqz
                                  br_if $block_17
                                  local.get $0
                                  i32.const 20
                                  i32.add
                                  local.get $0
                                  i32.const 16
                                  i32.add
                                  local.get $4
                                  select
                                  local.set $5
                                  block $block_18
                                    loop $loop
                                      local.get $5
                                      local.set $7
                                      block $block_19
                                        local.get $3
                                        local.tee $4
                                        i32.const 20
                                        i32.add
                                        local.tee $5
                                        i32.load
                                        local.tee $3
                                        i32.eqz
                                        br_if $block_19
                                        local.get $3
                                        br_if $loop
                                        br $block_18
                                      end ;; $block_19
                                      local.get $4
                                      i32.const 16
                                      i32.add
                                      local.set $5
                                      local.get $4
                                      i32.load offset=16
                                      local.tee $3
                                      br_if $loop
                                    end ;; $loop
                                  end ;; $block_18
                                  local.get $7
                                  i32.const 0
                                  i32.store
                                  local.get $6
                                  br_if $block_12
                                  br $block_11
                                end ;; $block_17
                                i32.const 0
                                local.set $4
                                local.get $6
                                i32.eqz
                                br_if $block_11
                              end ;; $block_12
                              block $block_20
                                block $block_21
                                  local.get $0
                                  i32.load offset=28
                                  i32.const 2
                                  i32.shl
                                  i32.const 1055408
                                  i32.add
                                  local.tee $3
                                  i32.load
                                  local.get $0
                                  i32.eq
                                  br_if $block_21
                                  local.get $6
                                  i32.const 16
                                  i32.const 20
                                  local.get $6
                                  i32.load offset=16
                                  local.get $0
                                  i32.eq
                                  select
                                  i32.add
                                  local.get $4
                                  i32.store
                                  local.get $4
                                  br_if $block_20
                                  br $block_11
                                end ;; $block_21
                                local.get $3
                                local.get $4
                                i32.store
                                local.get $4
                                i32.eqz
                                br_if $block_10
                              end ;; $block_20
                              local.get $4
                              local.get $6
                              i32.store offset=24
                              block $block_22
                                local.get $0
                                i32.load offset=16
                                local.tee $3
                                i32.eqz
                                br_if $block_22
                                local.get $4
                                local.get $3
                                i32.store offset=16
                                local.get $3
                                local.get $4
                                i32.store offset=24
                              end ;; $block_22
                              local.get $0
                              i32.load offset=20
                              local.tee $3
                              i32.eqz
                              br_if $block_11
                              local.get $4
                              i32.const 20
                              i32.add
                              local.get $3
                              i32.store
                              local.get $3
                              local.get $4
                              i32.store offset=24
                            end ;; $block_11
                            local.get $2
                            i32.load offset=4
                            local.tee $3
                            i32.const 2
                            i32.and
                            br_if $block_9
                            br $block_8
                          end ;; $block_10
                          i32.const 0
                          i32.const 0
                          i32.load offset=1055140
                          i32.const -2
                          local.get $0
                          i32.const 28
                          i32.add
                          i32.load
                          i32.rotl
                          i32.and
                          i32.store offset=1055140
                          local.get $2
                          i32.load offset=4
                          local.tee $3
                          i32.const 2
                          i32.and
                          i32.eqz
                          br_if $block_8
                        end ;; $block_9
                        local.get $2
                        i32.const 4
                        i32.add
                        local.get $3
                        i32.const -2
                        i32.and
                        i32.store
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
                        br $block
                      end ;; $block_8
                      block $block_23
                        i32.const 0
                        i32.load offset=1055548
                        local.get $2
                        i32.eq
                        br_if $block_23
                        i32.const 0
                        i32.load offset=1055544
                        local.get $2
                        i32.eq
                        br_if $block_6
                        local.get $3
                        i32.const -8
                        i32.and
                        local.tee $4
                        local.get $1
                        i32.add
                        local.set $1
                        local.get $4
                        i32.const 255
                        i32.gt_u
                        br_if $block_5
                        local.get $2
                        i32.load offset=12
                        local.tee $4
                        local.get $2
                        i32.load offset=8
                        local.tee $2
                        i32.eq
                        br_if $block_3
                        local.get $2
                        local.get $4
                        i32.store offset=12
                        local.get $4
                        local.get $2
                        i32.store offset=8
                        br $block_0
                      end ;; $block_23
                      i32.const 0
                      local.get $0
                      i32.store offset=1055548
                      i32.const 0
                      i32.const 0
                      i32.load offset=1055540
                      local.get $1
                      i32.add
                      local.tee $1
                      i32.store offset=1055540
                      local.get $0
                      local.get $1
                      i32.const 1
                      i32.or
                      i32.store offset=4
                      local.get $0
                      i32.const 0
                      i32.load offset=1055544
                      i32.eq
                      br_if $block_4
                    end ;; $block_7
                    return
                  end ;; $block_6
                  i32.const 0
                  local.get $0
                  i32.store offset=1055544
                  i32.const 0
                  i32.const 0
                  i32.load offset=1055536
                  local.get $1
                  i32.add
                  local.tee $1
                  i32.store offset=1055536
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
                end ;; $block_5
                local.get $2
                i32.load offset=24
                local.set $6
                local.get $2
                i32.load offset=12
                local.tee $4
                local.get $2
                i32.eq
                br_if $block_2
                local.get $2
                i32.load offset=8
                local.tee $3
                local.get $4
                i32.store offset=12
                local.get $4
                local.get $3
                i32.store offset=8
                local.get $6
                br_if $block_1
                br $block_0
              end ;; $block_4
              i32.const 0
              i32.const 0
              i32.store offset=1055536
              i32.const 0
              i32.const 0
              i32.store offset=1055544
              return
            end ;; $block_3
            i32.const 0
            i32.const 0
            i32.load offset=1055136
            i32.const -2
            local.get $3
            i32.const 3
            i32.shr_u
            i32.rotl
            i32.and
            i32.store offset=1055136
            br $block_0
          end ;; $block_2
          block $block_24
            local.get $2
            i32.const 20
            i32.const 16
            local.get $2
            i32.load offset=20
            local.tee $4
            select
            i32.add
            i32.load
            local.tee $3
            i32.eqz
            br_if $block_24
            local.get $2
            i32.const 20
            i32.add
            local.get $2
            i32.const 16
            i32.add
            local.get $4
            select
            local.set $5
            block $block_25
              loop $loop_0
                local.get $5
                local.set $7
                block $block_26
                  local.get $3
                  local.tee $4
                  i32.const 20
                  i32.add
                  local.tee $5
                  i32.load
                  local.tee $3
                  i32.eqz
                  br_if $block_26
                  local.get $3
                  br_if $loop_0
                  br $block_25
                end ;; $block_26
                local.get $4
                i32.const 16
                i32.add
                local.set $5
                local.get $4
                i32.load offset=16
                local.tee $3
                br_if $loop_0
              end ;; $loop_0
            end ;; $block_25
            local.get $7
            i32.const 0
            i32.store
            local.get $6
            br_if $block_1
            br $block_0
          end ;; $block_24
          i32.const 0
          local.set $4
          local.get $6
          i32.eqz
          br_if $block_0
        end ;; $block_1
        block $block_27
          block $block_28
            block $block_29
              local.get $2
              i32.load offset=28
              i32.const 2
              i32.shl
              i32.const 1055408
              i32.add
              local.tee $3
              i32.load
              local.get $2
              i32.eq
              br_if $block_29
              local.get $6
              i32.const 16
              i32.const 20
              local.get $6
              i32.load offset=16
              local.get $2
              i32.eq
              select
              i32.add
              local.get $4
              i32.store
              local.get $4
              br_if $block_28
              br $block_0
            end ;; $block_29
            local.get $3
            local.get $4
            i32.store
            local.get $4
            i32.eqz
            br_if $block_27
          end ;; $block_28
          local.get $4
          local.get $6
          i32.store offset=24
          block $block_30
            local.get $2
            i32.load offset=16
            local.tee $3
            i32.eqz
            br_if $block_30
            local.get $4
            local.get $3
            i32.store offset=16
            local.get $3
            local.get $4
            i32.store offset=24
          end ;; $block_30
          local.get $2
          i32.load offset=20
          local.tee $2
          i32.eqz
          br_if $block_0
          local.get $4
          i32.const 20
          i32.add
          local.get $2
          i32.store
          local.get $2
          local.get $4
          i32.store offset=24
          br $block_0
        end ;; $block_27
        i32.const 0
        i32.const 0
        i32.load offset=1055140
        i32.const -2
        local.get $2
        i32.const 28
        i32.add
        i32.load
        i32.rotl
        i32.and
        i32.store offset=1055140
      end ;; $block_0
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
      i32.load offset=1055544
      i32.ne
      br_if $block
      i32.const 0
      local.get $1
      i32.store offset=1055536
      return
    end ;; $block
    block $block_31
      block $block_32
        block $block_33
          block $block_34
            block $block_35
              block $block_36
                block $block_37
                  local.get $1
                  i32.const 255
                  i32.gt_u
                  br_if $block_37
                  local.get $1
                  i32.const 3
                  i32.shr_u
                  local.tee $2
                  i32.const 3
                  i32.shl
                  i32.const 1055144
                  i32.add
                  local.set $1
                  i32.const 0
                  i32.load offset=1055136
                  local.tee $3
                  i32.const 1
                  local.get $2
                  i32.const 31
                  i32.and
                  i32.shl
                  local.tee $2
                  i32.and
                  i32.eqz
                  br_if $block_36
                  local.get $1
                  i32.load offset=8
                  local.set $2
                  br $block_35
                end ;; $block_37
                i32.const 0
                local.set $2
                block $block_38
                  local.get $1
                  i32.const 8
                  i32.shr_u
                  local.tee $3
                  i32.eqz
                  br_if $block_38
                  i32.const 31
                  local.set $2
                  local.get $1
                  i32.const 16777215
                  i32.gt_u
                  br_if $block_38
                  local.get $1
                  i32.const 38
                  local.get $3
                  i32.clz
                  local.tee $2
                  i32.sub
                  i32.const 31
                  i32.and
                  i32.shr_u
                  i32.const 1
                  i32.and
                  i32.const 31
                  local.get $2
                  i32.sub
                  i32.const 1
                  i32.shl
                  i32.or
                  local.set $2
                end ;; $block_38
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
                i32.const 1055408
                i32.add
                local.set $3
                i32.const 0
                i32.load offset=1055140
                local.tee $4
                i32.const 1
                local.get $2
                i32.const 31
                i32.and
                i32.shl
                local.tee $5
                i32.and
                i32.eqz
                br_if $block_34
                local.get $3
                i32.load
                local.tee $4
                i32.load offset=4
                i32.const -8
                i32.and
                local.get $1
                i32.ne
                br_if $block_33
                local.get $4
                local.set $2
                br $block_32
              end ;; $block_36
              i32.const 0
              local.get $3
              local.get $2
              i32.or
              i32.store offset=1055136
              local.get $1
              local.set $2
            end ;; $block_35
            local.get $1
            i32.const 8
            i32.add
            local.get $0
            i32.store
            local.get $2
            local.get $0
            i32.store offset=12
            local.get $0
            local.get $1
            i32.store offset=12
            local.get $0
            local.get $2
            i32.store offset=8
            return
          end ;; $block_34
          i32.const 0
          local.get $4
          local.get $5
          i32.or
          i32.store offset=1055140
          local.get $3
          local.get $0
          i32.store
          local.get $0
          i32.const 24
          i32.add
          local.get $3
          i32.store
          local.get $0
          local.get $0
          i32.store offset=12
          local.get $0
          local.get $0
          i32.store offset=8
          return
        end ;; $block_33
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
          local.get $4
          local.get $3
          i32.const 29
          i32.shr_u
          i32.const 4
          i32.and
          i32.add
          i32.const 16
          i32.add
          local.tee $5
          i32.load
          local.tee $2
          i32.eqz
          br_if $block_31
          local.get $3
          i32.const 1
          i32.shl
          local.set $3
          local.get $2
          local.set $4
          local.get $2
          i32.load offset=4
          i32.const -8
          i32.and
          local.get $1
          i32.ne
          br_if $loop_1
        end ;; $loop_1
      end ;; $block_32
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
      return
    end ;; $block_31
    local.get $5
    local.get $0
    i32.store
    local.get $0
    i32.const 24
    i32.add
    local.get $4
    i32.store
    local.get $0
    local.get $0
    i32.store offset=12
    local.get $0
    local.get $0
    i32.store offset=8
    )
  
  (func $__rdl_dealloc (type $5)
    (param $0 i32)
    local.get $0
    call $dlmalloc::dlmalloc::Dlmalloc::free::h36961b6fbcc40c05
    )
  
  (func $dlmalloc::dlmalloc::Dlmalloc::free::h36961b6fbcc40c05 (type $5)
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
            block $block_3
              block $block_4
                block $block_5
                  local.get $2
                  i32.const 1
                  i32.and
                  br_if $block_5
                  local.get $2
                  i32.const 3
                  i32.and
                  i32.eqz
                  br_if $block
                  local.get $1
                  i32.load
                  local.tee $2
                  local.get $0
                  i32.add
                  local.set $0
                  block $block_6
                    block $block_7
                      block $block_8
                        block $block_9
                          block $block_10
                            i32.const 0
                            i32.load offset=1055544
                            local.get $1
                            local.get $2
                            i32.sub
                            local.tee $1
                            i32.eq
                            br_if $block_10
                            local.get $2
                            i32.const 255
                            i32.gt_u
                            br_if $block_9
                            local.get $1
                            i32.load offset=12
                            local.tee $4
                            local.get $1
                            i32.load offset=8
                            local.tee $5
                            i32.eq
                            br_if $block_8
                            local.get $5
                            local.get $4
                            i32.store offset=12
                            local.get $4
                            local.get $5
                            i32.store offset=8
                            local.get $3
                            i32.load offset=4
                            local.tee $2
                            i32.const 2
                            i32.and
                            i32.eqz
                            br_if $block_2
                            br $block_3
                          end ;; $block_10
                          local.get $3
                          i32.load offset=4
                          i32.const 3
                          i32.and
                          i32.const 3
                          i32.ne
                          br_if $block_5
                          i32.const 0
                          local.get $0
                          i32.store offset=1055536
                          local.get $3
                          i32.const 4
                          i32.add
                          local.tee $3
                          local.get $3
                          i32.load
                          i32.const -2
                          i32.and
                          i32.store
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
                        end ;; $block_9
                        local.get $1
                        i32.load offset=24
                        local.set $6
                        local.get $1
                        i32.load offset=12
                        local.tee $4
                        local.get $1
                        i32.eq
                        br_if $block_7
                        local.get $1
                        i32.load offset=8
                        local.tee $2
                        local.get $4
                        i32.store offset=12
                        local.get $4
                        local.get $2
                        i32.store offset=8
                        local.get $6
                        br_if $block_6
                        br $block_5
                      end ;; $block_8
                      i32.const 0
                      i32.const 0
                      i32.load offset=1055136
                      i32.const -2
                      local.get $2
                      i32.const 3
                      i32.shr_u
                      i32.rotl
                      i32.and
                      i32.store offset=1055136
                      local.get $3
                      i32.load offset=4
                      local.tee $2
                      i32.const 2
                      i32.and
                      i32.eqz
                      br_if $block_2
                      br $block_3
                    end ;; $block_7
                    block $block_11
                      local.get $1
                      i32.const 20
                      i32.const 16
                      local.get $1
                      i32.load offset=20
                      local.tee $4
                      select
                      i32.add
                      i32.load
                      local.tee $2
                      i32.eqz
                      br_if $block_11
                      local.get $1
                      i32.const 20
                      i32.add
                      local.get $1
                      i32.const 16
                      i32.add
                      local.get $4
                      select
                      local.set $5
                      block $block_12
                        loop $loop
                          local.get $5
                          local.set $7
                          block $block_13
                            local.get $2
                            local.tee $4
                            i32.const 20
                            i32.add
                            local.tee $5
                            i32.load
                            local.tee $2
                            i32.eqz
                            br_if $block_13
                            local.get $2
                            br_if $loop
                            br $block_12
                          end ;; $block_13
                          local.get $4
                          i32.const 16
                          i32.add
                          local.set $5
                          local.get $4
                          i32.load offset=16
                          local.tee $2
                          br_if $loop
                        end ;; $loop
                      end ;; $block_12
                      local.get $7
                      i32.const 0
                      i32.store
                      local.get $6
                      br_if $block_6
                      br $block_5
                    end ;; $block_11
                    i32.const 0
                    local.set $4
                    local.get $6
                    i32.eqz
                    br_if $block_5
                  end ;; $block_6
                  block $block_14
                    block $block_15
                      local.get $1
                      i32.load offset=28
                      i32.const 2
                      i32.shl
                      i32.const 1055408
                      i32.add
                      local.tee $2
                      i32.load
                      local.get $1
                      i32.eq
                      br_if $block_15
                      local.get $6
                      i32.const 16
                      i32.const 20
                      local.get $6
                      i32.load offset=16
                      local.get $1
                      i32.eq
                      select
                      i32.add
                      local.get $4
                      i32.store
                      local.get $4
                      br_if $block_14
                      br $block_5
                    end ;; $block_15
                    local.get $2
                    local.get $4
                    i32.store
                    local.get $4
                    i32.eqz
                    br_if $block_4
                  end ;; $block_14
                  local.get $4
                  local.get $6
                  i32.store offset=24
                  block $block_16
                    local.get $1
                    i32.load offset=16
                    local.tee $2
                    i32.eqz
                    br_if $block_16
                    local.get $4
                    local.get $2
                    i32.store offset=16
                    local.get $2
                    local.get $4
                    i32.store offset=24
                  end ;; $block_16
                  local.get $1
                  i32.load offset=20
                  local.tee $2
                  i32.eqz
                  br_if $block_5
                  local.get $4
                  i32.const 20
                  i32.add
                  local.get $2
                  i32.store
                  local.get $2
                  local.get $4
                  i32.store offset=24
                end ;; $block_5
                local.get $3
                i32.load offset=4
                local.tee $2
                i32.const 2
                i32.and
                br_if $block_3
                br $block_2
              end ;; $block_4
              i32.const 0
              i32.const 0
              i32.load offset=1055140
              i32.const -2
              local.get $1
              i32.const 28
              i32.add
              i32.load
              i32.rotl
              i32.and
              i32.store offset=1055140
              local.get $3
              i32.load offset=4
              local.tee $2
              i32.const 2
              i32.and
              i32.eqz
              br_if $block_2
            end ;; $block_3
            local.get $3
            i32.const 4
            i32.add
            local.get $2
            i32.const -2
            i32.and
            i32.store
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
            br $block_1
          end ;; $block_2
          block $block_17
            block $block_18
              block $block_19
                block $block_20
                  block $block_21
                    block $block_22
                      block $block_23
                        block $block_24
                          i32.const 0
                          i32.load offset=1055548
                          local.get $3
                          i32.eq
                          br_if $block_24
                          i32.const 0
                          i32.load offset=1055544
                          local.get $3
                          i32.eq
                          br_if $block_23
                          local.get $2
                          i32.const -8
                          i32.and
                          local.tee $4
                          local.get $0
                          i32.add
                          local.set $0
                          local.get $4
                          i32.const 255
                          i32.gt_u
                          br_if $block_22
                          local.get $3
                          i32.load offset=12
                          local.tee $4
                          local.get $3
                          i32.load offset=8
                          local.tee $3
                          i32.eq
                          br_if $block_20
                          local.get $3
                          local.get $4
                          i32.store offset=12
                          local.get $4
                          local.get $3
                          i32.store offset=8
                          br $block_17
                        end ;; $block_24
                        i32.const 0
                        local.get $1
                        i32.store offset=1055548
                        i32.const 0
                        i32.const 0
                        i32.load offset=1055540
                        local.get $0
                        i32.add
                        local.tee $0
                        i32.store offset=1055540
                        local.get $1
                        local.get $0
                        i32.const 1
                        i32.or
                        i32.store offset=4
                        local.get $1
                        i32.const 0
                        i32.load offset=1055544
                        i32.eq
                        br_if $block_21
                        i32.const 0
                        local.set $1
                        i32.const 0
                        i32.load offset=1055576
                        local.tee $2
                        local.get $0
                        i32.ge_u
                        br_if $block
                        br $block_0
                      end ;; $block_23
                      i32.const 0
                      local.get $1
                      i32.store offset=1055544
                      i32.const 0
                      i32.const 0
                      i32.load offset=1055536
                      local.get $0
                      i32.add
                      local.tee $0
                      i32.store offset=1055536
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
                    end ;; $block_22
                    local.get $3
                    i32.load offset=24
                    local.set $6
                    local.get $3
                    i32.load offset=12
                    local.tee $4
                    local.get $3
                    i32.eq
                    br_if $block_19
                    local.get $3
                    i32.load offset=8
                    local.tee $2
                    local.get $4
                    i32.store offset=12
                    local.get $4
                    local.get $2
                    i32.store offset=8
                    local.get $6
                    br_if $block_18
                    br $block_17
                  end ;; $block_21
                  i32.const 0
                  i32.const 0
                  i32.store offset=1055536
                  i32.const 0
                  i32.const 0
                  i32.store offset=1055544
                  i32.const 0
                  local.set $1
                  i32.const 0
                  i32.load offset=1055576
                  local.tee $2
                  local.get $0
                  i32.lt_u
                  br_if $block_0
                  br $block
                end ;; $block_20
                i32.const 0
                i32.const 0
                i32.load offset=1055136
                i32.const -2
                local.get $2
                i32.const 3
                i32.shr_u
                i32.rotl
                i32.and
                i32.store offset=1055136
                br $block_17
              end ;; $block_19
              block $block_25
                local.get $3
                i32.const 20
                i32.const 16
                local.get $3
                i32.load offset=20
                local.tee $4
                select
                i32.add
                i32.load
                local.tee $2
                i32.eqz
                br_if $block_25
                local.get $3
                i32.const 20
                i32.add
                local.get $3
                i32.const 16
                i32.add
                local.get $4
                select
                local.set $5
                block $block_26
                  loop $loop_0
                    local.get $5
                    local.set $7
                    block $block_27
                      local.get $2
                      local.tee $4
                      i32.const 20
                      i32.add
                      local.tee $5
                      i32.load
                      local.tee $2
                      i32.eqz
                      br_if $block_27
                      local.get $2
                      br_if $loop_0
                      br $block_26
                    end ;; $block_27
                    local.get $4
                    i32.const 16
                    i32.add
                    local.set $5
                    local.get $4
                    i32.load offset=16
                    local.tee $2
                    br_if $loop_0
                  end ;; $loop_0
                end ;; $block_26
                local.get $7
                i32.const 0
                i32.store
                local.get $6
                br_if $block_18
                br $block_17
              end ;; $block_25
              i32.const 0
              local.set $4
              local.get $6
              i32.eqz
              br_if $block_17
            end ;; $block_18
            block $block_28
              block $block_29
                block $block_30
                  local.get $3
                  i32.load offset=28
                  i32.const 2
                  i32.shl
                  i32.const 1055408
                  i32.add
                  local.tee $2
                  i32.load
                  local.get $3
                  i32.eq
                  br_if $block_30
                  local.get $6
                  i32.const 16
                  i32.const 20
                  local.get $6
                  i32.load offset=16
                  local.get $3
                  i32.eq
                  select
                  i32.add
                  local.get $4
                  i32.store
                  local.get $4
                  br_if $block_29
                  br $block_17
                end ;; $block_30
                local.get $2
                local.get $4
                i32.store
                local.get $4
                i32.eqz
                br_if $block_28
              end ;; $block_29
              local.get $4
              local.get $6
              i32.store offset=24
              block $block_31
                local.get $3
                i32.load offset=16
                local.tee $2
                i32.eqz
                br_if $block_31
                local.get $4
                local.get $2
                i32.store offset=16
                local.get $2
                local.get $4
                i32.store offset=24
              end ;; $block_31
              local.get $3
              i32.load offset=20
              local.tee $3
              i32.eqz
              br_if $block_17
              local.get $4
              i32.const 20
              i32.add
              local.get $3
              i32.store
              local.get $3
              local.get $4
              i32.store offset=24
              br $block_17
            end ;; $block_28
            i32.const 0
            i32.const 0
            i32.load offset=1055140
            i32.const -2
            local.get $3
            i32.const 28
            i32.add
            i32.load
            i32.rotl
            i32.and
            i32.store offset=1055140
          end ;; $block_17
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
          i32.load offset=1055544
          i32.ne
          br_if $block_1
          i32.const 0
          local.get $0
          i32.store offset=1055536
          return
        end ;; $block_1
        block $block_32
          block $block_33
            block $block_34
              block $block_35
                block $block_36
                  block $block_37
                    block $block_38
                      block $block_39
                        block $block_40
                          local.get $0
                          i32.const 255
                          i32.gt_u
                          br_if $block_40
                          local.get $0
                          i32.const 3
                          i32.shr_u
                          local.tee $3
                          i32.const 3
                          i32.shl
                          i32.const 1055144
                          i32.add
                          local.set $0
                          i32.const 0
                          i32.load offset=1055136
                          local.tee $2
                          i32.const 1
                          local.get $3
                          i32.const 31
                          i32.and
                          i32.shl
                          local.tee $3
                          i32.and
                          i32.eqz
                          br_if $block_39
                          local.get $0
                          i32.const 8
                          i32.add
                          local.set $2
                          local.get $0
                          i32.load offset=8
                          local.set $3
                          br $block_38
                        end ;; $block_40
                        i32.const 0
                        local.set $3
                        block $block_41
                          local.get $0
                          i32.const 8
                          i32.shr_u
                          local.tee $2
                          i32.eqz
                          br_if $block_41
                          i32.const 31
                          local.set $3
                          local.get $0
                          i32.const 16777215
                          i32.gt_u
                          br_if $block_41
                          local.get $0
                          i32.const 38
                          local.get $2
                          i32.clz
                          local.tee $3
                          i32.sub
                          i32.const 31
                          i32.and
                          i32.shr_u
                          i32.const 1
                          i32.and
                          i32.const 31
                          local.get $3
                          i32.sub
                          i32.const 1
                          i32.shl
                          i32.or
                          local.set $3
                        end ;; $block_41
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
                        i32.const 1055408
                        i32.add
                        local.set $2
                        i32.const 0
                        i32.load offset=1055140
                        local.tee $4
                        i32.const 1
                        local.get $3
                        i32.const 31
                        i32.and
                        i32.shl
                        local.tee $5
                        i32.and
                        i32.eqz
                        br_if $block_37
                        local.get $2
                        i32.load
                        local.tee $4
                        i32.load offset=4
                        i32.const -8
                        i32.and
                        local.get $0
                        i32.ne
                        br_if $block_36
                        local.get $4
                        local.set $3
                        br $block_35
                      end ;; $block_39
                      i32.const 0
                      local.get $2
                      local.get $3
                      i32.or
                      i32.store offset=1055136
                      local.get $0
                      i32.const 8
                      i32.add
                      local.set $2
                      local.get $0
                      local.set $3
                    end ;; $block_38
                    local.get $2
                    local.get $1
                    i32.store
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
                  end ;; $block_37
                  i32.const 0
                  local.get $4
                  local.get $5
                  i32.or
                  i32.store offset=1055140
                  local.get $2
                  local.get $1
                  i32.store
                  local.get $1
                  i32.const 24
                  i32.add
                  local.get $2
                  i32.store
                  br $block_33
                end ;; $block_36
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
                loop $loop_1
                  local.get $4
                  local.get $2
                  i32.const 29
                  i32.shr_u
                  i32.const 4
                  i32.and
                  i32.add
                  i32.const 16
                  i32.add
                  local.tee $5
                  i32.load
                  local.tee $3
                  i32.eqz
                  br_if $block_34
                  local.get $2
                  i32.const 1
                  i32.shl
                  local.set $2
                  local.get $3
                  local.set $4
                  local.get $3
                  i32.load offset=4
                  i32.const -8
                  i32.and
                  local.get $0
                  i32.ne
                  br_if $loop_1
                end ;; $loop_1
              end ;; $block_35
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
              br $block_32
            end ;; $block_34
            local.get $5
            local.get $1
            i32.store
            local.get $1
            i32.const 24
            i32.add
            local.get $4
            i32.store
          end ;; $block_33
          local.get $1
          local.get $1
          i32.store offset=12
          local.get $1
          local.get $1
          i32.store offset=8
        end ;; $block_32
        i32.const 0
        i32.const 0
        i32.load offset=1055584
        i32.const -1
        i32.add
        local.tee $1
        i32.store offset=1055584
        local.get $1
        br_if $block
        block $block_42
          i32.const 0
          i32.load offset=1055568
          local.tee $0
          i32.eqz
          br_if $block_42
          i32.const 0
          local.set $1
          loop $loop_2
            local.get $1
            i32.const 1
            i32.add
            local.set $1
            local.get $0
            i32.load offset=8
            local.tee $0
            br_if $loop_2
          end ;; $loop_2
          i32.const 0
          local.get $1
          i32.const 4095
          local.get $1
          i32.const 4095
          i32.gt_u
          select
          i32.store offset=1055584
          return
        end ;; $block_42
        i32.const 0
        i32.const 4095
        i32.store offset=1055584
        return
      end ;; $block_0
      local.get $1
      i32.load offset=1055548
      local.tee $0
      i32.eqz
      br_if $block
      block $block_43
        i32.const 0
        i32.load offset=1055540
        local.tee $4
        i32.const 41
        i32.lt_u
        br_if $block_43
        i32.const 1055560
        local.set $1
        loop $loop_3
          block $block_44
            local.get $1
            i32.load
            local.tee $3
            local.get $0
            i32.gt_u
            br_if $block_44
            local.get $3
            local.get $1
            i32.load offset=4
            i32.add
            local.get $0
            i32.gt_u
            br_if $block_43
          end ;; $block_44
          local.get $1
          i32.load offset=8
          local.tee $1
          br_if $loop_3
        end ;; $loop_3
      end ;; $block_43
      block $block_45
        block $block_46
          i32.const 0
          i32.load offset=1055568
          local.tee $0
          i32.eqz
          br_if $block_46
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
          i32.const 0
          local.set $0
          i32.const 0
          local.get $1
          i32.const 4095
          local.get $1
          i32.const 4095
          i32.gt_u
          select
          i32.store offset=1055584
          local.get $4
          local.get $2
          i32.gt_u
          br_if $block_45
          br $block
        end ;; $block_46
        i32.const 0
        local.set $0
        i32.const 0
        i32.const 4095
        i32.store offset=1055584
        local.get $4
        local.get $2
        i32.le_u
        br_if $block
      end ;; $block_45
      local.get $0
      i32.const -1
      i32.store offset=1055576
      return
    end ;; $block
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
      block $block_0
        block $block_1
          block $block_2
            block $block_3
              block $block_4
                block $block_5
                  block $block_6
                    block $block_7
                      local.get $1
                      i32.const -65
                      i32.gt_u
                      br_if $block_7
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
                      block $block_8
                        block $block_9
                          block $block_10
                            block $block_11
                              block $block_12
                                local.get $5
                                i32.const 3
                                i32.and
                                i32.eqz
                                br_if $block_12
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
                                br_if $block_11
                                i32.const 0
                                i32.load offset=1055548
                                local.get $8
                                i32.eq
                                br_if $block_10
                                i32.const 0
                                i32.load offset=1055544
                                local.get $8
                                i32.eq
                                br_if $block_9
                                local.get $8
                                i32.load offset=4
                                local.tee $5
                                i32.const 2
                                i32.and
                                br_if $block_8
                                local.get $5
                                i32.const -8
                                i32.and
                                local.tee $9
                                local.get $6
                                i32.add
                                local.tee $10
                                local.get $3
                                i32.lt_u
                                br_if $block_8
                                local.get $10
                                local.get $3
                                i32.sub
                                local.set $11
                                local.get $9
                                i32.const 255
                                i32.gt_u
                                br_if $block_5
                                local.get $8
                                i32.load offset=12
                                local.tee $1
                                local.get $8
                                i32.load offset=8
                                local.tee $2
                                i32.eq
                                br_if $block_4
                                local.get $2
                                local.get $1
                                i32.store offset=12
                                local.get $1
                                local.get $2
                                i32.store offset=8
                                local.get $11
                                i32.const 15
                                i32.le_u
                                br_if $block_0
                                br $block
                              end ;; $block_12
                              local.get $3
                              i32.const 256
                              i32.lt_u
                              br_if $block_8
                              local.get $6
                              local.get $3
                              i32.const 4
                              i32.or
                              i32.lt_u
                              br_if $block_8
                              local.get $6
                              local.get $3
                              i32.sub
                              i32.const 131073
                              i32.ge_u
                              br_if $block_8
                              local.get $0
                              return
                            end ;; $block_11
                            block $block_13
                              local.get $6
                              local.get $3
                              i32.sub
                              local.tee $1
                              i32.const 16
                              i32.ge_u
                              br_if $block_13
                              local.get $0
                              return
                            end ;; $block_13
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
                            call $dlmalloc::dlmalloc::Dlmalloc::dispose_chunk::hf54af7e93e82e2ab
                            local.get $0
                            return
                          end ;; $block_10
                          i32.const 0
                          i32.load offset=1055540
                          local.get $6
                          i32.add
                          local.tee $6
                          local.get $3
                          i32.le_u
                          br_if $block_8
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
                          i32.store offset=1055540
                          i32.const 0
                          local.get $1
                          i32.store offset=1055548
                          local.get $0
                          return
                        end ;; $block_9
                        i32.const 0
                        i32.load offset=1055536
                        local.get $6
                        i32.add
                        local.tee $6
                        local.get $3
                        i32.ge_u
                        br_if $block_6
                      end ;; $block_8
                      local.get $1
                      call $dlmalloc::dlmalloc::Dlmalloc::malloc::hb5084b40b14605ec
                      local.tee $3
                      i32.eqz
                      br_if $block_7
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
                      call $dlmalloc::dlmalloc::Dlmalloc::free::h36961b6fbcc40c05
                      local.get $1
                      local.set $2
                    end ;; $block_7
                    local.get $2
                    return
                  end ;; $block_6
                  block $block_14
                    block $block_15
                      local.get $6
                      local.get $3
                      i32.sub
                      local.tee $1
                      i32.const 16
                      i32.ge_u
                      br_if $block_15
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
                      br $block_14
                    end ;; $block_15
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
                  end ;; $block_14
                  i32.const 0
                  local.get $2
                  i32.store offset=1055544
                  i32.const 0
                  local.get $1
                  i32.store offset=1055536
                  local.get $0
                  return
                end ;; $block_5
                local.get $8
                i32.load offset=24
                local.set $9
                local.get $8
                i32.load offset=12
                local.tee $2
                local.get $8
                i32.eq
                br_if $block_3
                local.get $8
                i32.load offset=8
                local.tee $1
                local.get $2
                i32.store offset=12
                local.get $2
                local.get $1
                i32.store offset=8
                local.get $9
                br_if $block_2
                br $block_1
              end ;; $block_4
              i32.const 0
              i32.const 0
              i32.load offset=1055136
              i32.const -2
              local.get $5
              i32.const 3
              i32.shr_u
              i32.rotl
              i32.and
              i32.store offset=1055136
              local.get $11
              i32.const 15
              i32.le_u
              br_if $block_0
              br $block
            end ;; $block_3
            block $block_16
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
              i32.eqz
              br_if $block_16
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
                block $block_17
                  local.get $1
                  local.tee $2
                  i32.const 20
                  i32.add
                  local.tee $6
                  i32.load
                  local.tee $1
                  br_if $block_17
                  local.get $2
                  i32.const 16
                  i32.add
                  local.set $6
                  local.get $2
                  i32.load offset=16
                  local.set $1
                end ;; $block_17
                local.get $1
                br_if $loop
              end ;; $loop
              local.get $5
              i32.const 0
              i32.store
              local.get $9
              br_if $block_2
              br $block_1
            end ;; $block_16
            i32.const 0
            local.set $2
            local.get $9
            i32.eqz
            br_if $block_1
          end ;; $block_2
          block $block_18
            block $block_19
              block $block_20
                local.get $8
                i32.load offset=28
                i32.const 2
                i32.shl
                i32.const 1055408
                i32.add
                local.tee $1
                i32.load
                local.get $8
                i32.eq
                br_if $block_20
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
                br_if $block_19
                br $block_1
              end ;; $block_20
              local.get $1
              local.get $2
              i32.store
              local.get $2
              i32.eqz
              br_if $block_18
            end ;; $block_19
            local.get $2
            local.get $9
            i32.store offset=24
            block $block_21
              local.get $8
              i32.load offset=16
              local.tee $1
              i32.eqz
              br_if $block_21
              local.get $2
              local.get $1
              i32.store offset=16
              local.get $1
              local.get $2
              i32.store offset=24
            end ;; $block_21
            local.get $8
            i32.load offset=20
            local.tee $1
            i32.eqz
            br_if $block_1
            local.get $2
            i32.const 20
            i32.add
            local.get $1
            i32.store
            local.get $1
            local.get $2
            i32.store offset=24
            local.get $11
            i32.const 15
            i32.le_u
            br_if $block_0
            br $block
          end ;; $block_18
          i32.const 0
          i32.const 0
          i32.load offset=1055140
          i32.const -2
          local.get $8
          i32.const 28
          i32.add
          i32.load
          i32.rotl
          i32.and
          i32.store offset=1055140
        end ;; $block_1
        local.get $11
        i32.const 15
        i32.gt_u
        br_if $block
      end ;; $block_0
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
      local.get $0
      return
    end ;; $block
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
    call $dlmalloc::dlmalloc::Dlmalloc::dispose_chunk::hf54af7e93e82e2ab
    local.get $0
    )
  
  (func $rust_begin_unwind (type $5)
    (param $0 i32)
    local.get $0
    call $std::panicking::continue_panic_fmt::h9d276daacc0ab278
    unreachable
    )
  
  (func $core::slice::slice_index_len_fail::h7a4ef59ca849a060 (type $0)
    (param $0 i32)
    (param $1 i32)
    (local $2 i32)
    global.get $12
    i32.const 48
    i32.sub
    local.tee $2
    global.set $12
    local.get $2
    local.get $1
    i32.store offset=4
    local.get $2
    local.get $0
    i32.store
    local.get $2
    i32.const 32
    i32.add
    i32.const 12
    i32.add
    i32.const 6
    i32.store
    local.get $2
    i32.const 28
    i32.add
    i32.const 2
    i32.store
    local.get $2
    i32.const 8
    i32.add
    i32.const 12
    i32.add
    i32.const 2
    i32.store
    local.get $2
    i32.const 6
    i32.store offset=36
    local.get $2
    i32.const 1048928
    i32.store offset=16
    local.get $2
    i32.const 2
    i32.store offset=12
    local.get $2
    i32.const 1053192
    i32.store offset=8
    local.get $2
    local.get $2
    i32.const 4
    i32.add
    i32.store offset=40
    local.get $2
    local.get $2
    i32.store offset=32
    local.get $2
    local.get $2
    i32.const 32
    i32.add
    i32.store offset=24
    local.get $2
    i32.const 8
    i32.add
    i32.const 1053208
    call $core::panicking::panic_fmt::h74ee8034b317ceed
    unreachable
    )
  
  (func $core::fmt::num::<impl_core::fmt::Display_for_u32>::fmt::h4f4d56a15c6233ed (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i32)
    (local $6 i32)
    (local $7 i32)
    global.get $12
    i32.const 48
    i32.sub
    local.tee $2
    global.set $12
    i32.const 39
    local.set $3
    block $block
      block $block_0
        block $block_1
          block $block_2
            block $block_3
              block $block_4
                local.get $0
                i32.load
                local.tee $0
                i32.const 10000
                i32.lt_u
                br_if $block_4
                i32.const 39
                local.set $3
                loop $loop
                  local.get $2
                  i32.const 9
                  i32.add
                  local.get $3
                  i32.add
                  local.tee $4
                  i32.const -4
                  i32.add
                  local.get $0
                  local.get $0
                  i32.const 10000
                  i32.div_u
                  local.tee $5
                  i32.const -10000
                  i32.mul
                  i32.add
                  local.tee $6
                  i32.const 100
                  i32.div_u
                  local.tee $7
                  i32.const 1
                  i32.shl
                  i32.const 1049050
                  i32.add
                  i32.load16_u align=1
                  i32.store16 align=1
                  local.get $4
                  i32.const -2
                  i32.add
                  local.get $6
                  local.get $7
                  i32.const -100
                  i32.mul
                  i32.add
                  i32.const 1
                  i32.shl
                  i32.const 1049050
                  i32.add
                  i32.load16_u align=1
                  i32.store16 align=1
                  local.get $3
                  i32.const -4
                  i32.add
                  local.set $3
                  local.get $0
                  i32.const 99999999
                  i32.gt_u
                  local.set $4
                  local.get $5
                  local.set $0
                  local.get $4
                  br_if $loop
                end ;; $loop
                i32.const 100
                local.set $4
                local.get $5
                i32.const 100
                i32.lt_s
                br_if $block_3
                br $block_2
              end ;; $block_4
              i32.const 100
              local.set $4
              local.get $0
              local.tee $5
              i32.const 100
              i32.ge_s
              br_if $block_2
            end ;; $block_3
            local.get $5
            local.tee $0
            i32.const 9
            i32.gt_s
            br_if $block_1
            br $block_0
          end ;; $block_2
          local.get $2
          i32.const 9
          i32.add
          local.get $3
          i32.const -2
          i32.add
          local.tee $3
          i32.add
          local.get $5
          i32.const 65535
          i32.and
          local.get $4
          i32.div_u
          local.tee $0
          i32.const -100
          i32.mul
          local.get $5
          i32.add
          i32.const 65535
          i32.and
          i32.const 1
          i32.shl
          i32.const 1049050
          i32.add
          i32.load16_u align=1
          i32.store16 align=1
          local.get $0
          i32.const 9
          i32.le_s
          br_if $block_0
        end ;; $block_1
        local.get $2
        i32.const 9
        i32.add
        local.get $3
        i32.const -2
        i32.add
        local.tee $3
        i32.add
        local.tee $5
        local.get $0
        i32.const 1
        i32.shl
        i32.const 1049050
        i32.add
        i32.load16_u align=1
        i32.store16 align=1
        br $block
      end ;; $block_0
      local.get $2
      i32.const 9
      i32.add
      local.get $3
      i32.const -1
      i32.add
      local.tee $3
      i32.add
      local.tee $5
      local.get $0
      i32.const 48
      i32.add
      i32.store8
    end ;; $block
    local.get $1
    i32.const 1049000
    i32.const 0
    local.get $5
    i32.const 39
    local.get $3
    i32.sub
    call $core::fmt::Formatter::pad_integral::hb6042588a2a3edea
    local.set $0
    local.get $2
    i32.const 48
    i32.add
    global.set $12
    local.get $0
    )
  
  (func $core::panicking::panic_bounds_check::h34b80e64d41db052 (type $3)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (local $3 i32)
    global.get $12
    i32.const 48
    i32.sub
    local.tee $3
    global.set $12
    local.get $3
    local.get $2
    i32.store offset=4
    local.get $3
    local.get $1
    i32.store
    local.get $3
    i32.const 32
    i32.add
    i32.const 12
    i32.add
    i32.const 6
    i32.store
    local.get $3
    i32.const 28
    i32.add
    i32.const 2
    i32.store
    local.get $3
    i32.const 8
    i32.add
    i32.const 12
    i32.add
    i32.const 2
    i32.store
    local.get $3
    i32.const 6
    i32.store offset=36
    local.get $3
    i32.const 1048928
    i32.store offset=16
    local.get $3
    i32.const 2
    i32.store offset=12
    local.get $3
    i32.const 1053160
    i32.store offset=8
    local.get $3
    local.get $3
    i32.store offset=40
    local.get $3
    local.get $3
    i32.const 4
    i32.add
    i32.store offset=32
    local.get $3
    local.get $3
    i32.const 32
    i32.add
    i32.store offset=24
    local.get $3
    i32.const 8
    i32.add
    local.get $0
    call $core::panicking::panic_fmt::h74ee8034b317ceed
    unreachable
    )
  
  (func $core::fmt::Formatter::pad_integral::hb6042588a2a3edea (type $7)
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
    (local $11 i32)
    (local $12 i32)
    (local $13 i32)
    global.get $12
    i32.const 16
    i32.sub
    local.tee $5
    global.set $12
    local.get $0
    i32.load
    local.tee $6
    i32.const 1
    i32.and
    local.tee $7
    local.get $4
    i32.add
    local.set $8
    block $block
      local.get $6
      i32.const 4
      i32.and
      local.tee $9
      i32.eqz
      br_if $block
      i32.const 0
      local.set $10
      block $block_0
        local.get $2
        i32.eqz
        br_if $block_0
        local.get $2
        local.set $11
        local.get $1
        local.set $12
        loop $loop
          local.get $10
          local.get $12
          i32.load8_u
          i32.const 192
          i32.and
          i32.const 128
          i32.eq
          i32.add
          local.set $10
          local.get $12
          i32.const 1
          i32.add
          local.set $12
          local.get $11
          i32.const -1
          i32.add
          local.tee $11
          br_if $loop
        end ;; $loop
      end ;; $block_0
      local.get $8
      local.get $2
      i32.add
      local.get $10
      i32.sub
      local.set $8
    end ;; $block
    i32.const 43
    i32.const 1114112
    local.get $7
    select
    local.set $13
    local.get $9
    i32.const 2
    i32.shr_u
    local.set $9
    block $block_1
      block $block_2
        block $block_3
          block $block_4
            block $block_5
              block $block_6
                block $block_7
                  block $block_8
                    block $block_9
                      block $block_10
                        block $block_11
                          block $block_12
                            block $block_13
                              block $block_14
                                block $block_15
                                  block $block_16
                                    block $block_17
                                      block $block_18
                                        local.get $0
                                        i32.load offset=8
                                        i32.const 1
                                        i32.ne
                                        br_if $block_18
                                        local.get $0
                                        i32.const 12
                                        i32.add
                                        i32.load
                                        local.tee $10
                                        local.get $8
                                        i32.le_u
                                        br_if $block_17
                                        local.get $6
                                        i32.const 8
                                        i32.and
                                        br_if $block_16
                                        local.get $10
                                        local.get $8
                                        i32.sub
                                        local.set $11
                                        i32.const 1
                                        local.get $0
                                        i32.load8_u offset=48
                                        local.tee $12
                                        local.get $12
                                        i32.const 3
                                        i32.eq
                                        select
                                        i32.const 3
                                        i32.and
                                        local.tee $12
                                        i32.eqz
                                        br_if $block_15
                                        local.get $12
                                        i32.const 2
                                        i32.eq
                                        br_if $block_14
                                        i32.const 0
                                        local.set $8
                                        br $block_13
                                      end ;; $block_18
                                      block $block_19
                                        local.get $7
                                        i32.eqz
                                        br_if $block_19
                                        i32.const 1
                                        local.set $12
                                        local.get $0
                                        i32.const 24
                                        i32.add
                                        i32.load
                                        local.get $13
                                        local.get $0
                                        i32.const 28
                                        i32.add
                                        i32.load
                                        i32.load offset=16
                                        call_indirect $10 (type $2)
                                        br_if $block_2
                                      end ;; $block_19
                                      block $block_20
                                        local.get $9
                                        i32.eqz
                                        br_if $block_20
                                        i32.const 1
                                        local.set $12
                                        local.get $0
                                        i32.const 24
                                        i32.add
                                        i32.load
                                        local.get $1
                                        local.get $2
                                        local.get $0
                                        i32.const 28
                                        i32.add
                                        i32.load
                                        i32.load offset=12
                                        call_indirect $10 (type $1)
                                        br_if $block_2
                                      end ;; $block_20
                                      local.get $0
                                      i32.const 24
                                      i32.add
                                      i32.load
                                      local.get $3
                                      local.get $4
                                      local.get $0
                                      i32.const 28
                                      i32.add
                                      i32.load
                                      i32.load offset=12
                                      call_indirect $10 (type $1)
                                      local.set $12
                                      local.get $5
                                      i32.const 16
                                      i32.add
                                      global.set $12
                                      local.get $12
                                      return
                                    end ;; $block_17
                                    block $block_21
                                      local.get $7
                                      i32.eqz
                                      br_if $block_21
                                      i32.const 1
                                      local.set $12
                                      local.get $0
                                      i32.const 24
                                      i32.add
                                      i32.load
                                      local.get $13
                                      local.get $0
                                      i32.const 28
                                      i32.add
                                      i32.load
                                      i32.load offset=16
                                      call_indirect $10 (type $2)
                                      br_if $block_2
                                    end ;; $block_21
                                    block $block_22
                                      local.get $9
                                      i32.eqz
                                      br_if $block_22
                                      i32.const 1
                                      local.set $12
                                      local.get $0
                                      i32.const 24
                                      i32.add
                                      i32.load
                                      local.get $1
                                      local.get $2
                                      local.get $0
                                      i32.const 28
                                      i32.add
                                      i32.load
                                      i32.load offset=12
                                      call_indirect $10 (type $1)
                                      br_if $block_2
                                    end ;; $block_22
                                    local.get $0
                                    i32.const 24
                                    i32.add
                                    i32.load
                                    local.get $3
                                    local.get $4
                                    local.get $0
                                    i32.const 28
                                    i32.add
                                    i32.load
                                    i32.load offset=12
                                    call_indirect $10 (type $1)
                                    local.set $12
                                    local.get $5
                                    i32.const 16
                                    i32.add
                                    global.set $12
                                    local.get $12
                                    return
                                  end ;; $block_16
                                  i32.const 1
                                  local.set $12
                                  local.get $0
                                  i32.const 1
                                  i32.store8 offset=48
                                  local.get $0
                                  i32.const 48
                                  i32.store offset=4
                                  block $block_23
                                    local.get $7
                                    i32.eqz
                                    br_if $block_23
                                    local.get $0
                                    i32.load offset=24
                                    local.get $13
                                    local.get $0
                                    i32.const 28
                                    i32.add
                                    i32.load
                                    i32.load offset=16
                                    call_indirect $10 (type $2)
                                    br_if $block_2
                                  end ;; $block_23
                                  block $block_24
                                    local.get $9
                                    i32.eqz
                                    br_if $block_24
                                    local.get $0
                                    i32.load offset=24
                                    local.get $1
                                    local.get $2
                                    local.get $0
                                    i32.const 28
                                    i32.add
                                    i32.load
                                    i32.load offset=12
                                    call_indirect $10 (type $1)
                                    br_if $block_2
                                  end ;; $block_24
                                  local.get $10
                                  local.get $8
                                  i32.sub
                                  local.set $11
                                  i32.const 1
                                  local.get $0
                                  i32.const 48
                                  i32.add
                                  i32.load8_u
                                  local.tee $12
                                  local.get $12
                                  i32.const 3
                                  i32.eq
                                  select
                                  i32.const 3
                                  i32.and
                                  local.tee $12
                                  i32.eqz
                                  br_if $block_11
                                  local.get $12
                                  i32.const 2
                                  i32.eq
                                  br_if $block_12
                                  i32.const 0
                                  local.set $7
                                  br $block_10
                                end ;; $block_15
                                local.get $11
                                local.set $8
                                i32.const 0
                                local.set $11
                                br $block_13
                              end ;; $block_14
                              local.get $11
                              i32.const 1
                              i32.add
                              i32.const 1
                              i32.shr_u
                              local.set $8
                              local.get $11
                              i32.const 1
                              i32.shr_u
                              local.set $11
                            end ;; $block_13
                            local.get $5
                            i32.const 0
                            i32.store offset=12
                            block $block_25
                              local.get $0
                              i32.load offset=4
                              local.tee $12
                              i32.const 127
                              i32.gt_u
                              br_if $block_25
                              local.get $5
                              local.get $12
                              i32.store8 offset=12
                              i32.const 1
                              local.set $10
                              br $block_8
                            end ;; $block_25
                            block $block_26
                              local.get $12
                              i32.const 2047
                              i32.gt_u
                              br_if $block_26
                              local.get $5
                              local.get $12
                              i32.const 63
                              i32.and
                              i32.const 128
                              i32.or
                              i32.store8 offset=13
                              local.get $5
                              local.get $12
                              i32.const 6
                              i32.shr_u
                              i32.const 31
                              i32.and
                              i32.const 192
                              i32.or
                              i32.store8 offset=12
                              i32.const 2
                              local.set $10
                              br $block_8
                            end ;; $block_26
                            local.get $12
                            i32.const 65535
                            i32.gt_u
                            br_if $block_9
                            local.get $5
                            local.get $12
                            i32.const 63
                            i32.and
                            i32.const 128
                            i32.or
                            i32.store8 offset=14
                            local.get $5
                            local.get $12
                            i32.const 6
                            i32.shr_u
                            i32.const 63
                            i32.and
                            i32.const 128
                            i32.or
                            i32.store8 offset=13
                            local.get $5
                            local.get $12
                            i32.const 12
                            i32.shr_u
                            i32.const 15
                            i32.and
                            i32.const 224
                            i32.or
                            i32.store8 offset=12
                            i32.const 3
                            local.set $10
                            br $block_8
                          end ;; $block_12
                          local.get $11
                          i32.const 1
                          i32.add
                          i32.const 1
                          i32.shr_u
                          local.set $7
                          local.get $11
                          i32.const 1
                          i32.shr_u
                          local.set $11
                          br $block_10
                        end ;; $block_11
                        local.get $11
                        local.set $7
                        i32.const 0
                        local.set $11
                      end ;; $block_10
                      local.get $5
                      i32.const 0
                      i32.store offset=8
                      block $block_27
                        local.get $0
                        i32.const 4
                        i32.add
                        i32.load
                        local.tee $12
                        i32.const 127
                        i32.gt_u
                        br_if $block_27
                        local.get $5
                        local.get $12
                        i32.store8 offset=8
                        i32.const 1
                        local.set $10
                        br $block_4
                      end ;; $block_27
                      local.get $12
                      i32.const 2047
                      i32.gt_u
                      br_if $block_7
                      local.get $5
                      local.get $12
                      i32.const 63
                      i32.and
                      i32.const 128
                      i32.or
                      i32.store8 offset=9
                      local.get $5
                      local.get $12
                      i32.const 6
                      i32.shr_u
                      i32.const 31
                      i32.and
                      i32.const 192
                      i32.or
                      i32.store8 offset=8
                      i32.const 2
                      local.set $10
                      br $block_4
                    end ;; $block_9
                    local.get $5
                    local.get $12
                    i32.const 63
                    i32.and
                    i32.const 128
                    i32.or
                    i32.store8 offset=15
                    local.get $5
                    local.get $12
                    i32.const 18
                    i32.shr_u
                    i32.const 240
                    i32.or
                    i32.store8 offset=12
                    local.get $5
                    local.get $12
                    i32.const 6
                    i32.shr_u
                    i32.const 63
                    i32.and
                    i32.const 128
                    i32.or
                    i32.store8 offset=14
                    local.get $5
                    local.get $12
                    i32.const 12
                    i32.shr_u
                    i32.const 63
                    i32.and
                    i32.const 128
                    i32.or
                    i32.store8 offset=13
                    i32.const 4
                    local.set $10
                  end ;; $block_8
                  i32.const -1
                  local.set $12
                  block $block_28
                    loop $loop_0
                      local.get $12
                      i32.const 1
                      i32.add
                      local.tee $12
                      local.get $11
                      i32.ge_u
                      br_if $block_28
                      local.get $0
                      i32.const 24
                      i32.add
                      i32.load
                      local.get $5
                      i32.const 12
                      i32.add
                      local.get $10
                      local.get $0
                      i32.const 28
                      i32.add
                      i32.load
                      i32.load offset=12
                      call_indirect $10 (type $1)
                      i32.eqz
                      br_if $loop_0
                      br $block_3
                    end ;; $loop_0
                  end ;; $block_28
                  block $block_29
                    local.get $7
                    i32.eqz
                    br_if $block_29
                    local.get $0
                    i32.const 24
                    i32.add
                    i32.load
                    local.get $13
                    local.get $0
                    i32.const 28
                    i32.add
                    i32.load
                    i32.load offset=16
                    call_indirect $10 (type $2)
                    br_if $block_3
                  end ;; $block_29
                  block $block_30
                    local.get $9
                    i32.eqz
                    br_if $block_30
                    local.get $0
                    i32.const 24
                    i32.add
                    i32.load
                    local.get $1
                    local.get $2
                    local.get $0
                    i32.const 28
                    i32.add
                    i32.load
                    i32.load offset=12
                    call_indirect $10 (type $1)
                    br_if $block_3
                  end ;; $block_30
                  local.get $0
                  i32.const 24
                  i32.add
                  local.tee $11
                  i32.load
                  local.get $3
                  local.get $4
                  local.get $0
                  i32.const 28
                  i32.add
                  local.tee $0
                  i32.load
                  i32.load offset=12
                  call_indirect $10 (type $1)
                  br_if $block_3
                  i32.const -1
                  local.set $12
                  loop $loop_1
                    local.get $12
                    i32.const 1
                    i32.add
                    local.tee $12
                    local.get $8
                    i32.ge_u
                    br_if $block_6
                    local.get $11
                    i32.load
                    local.get $5
                    i32.const 12
                    i32.add
                    local.get $10
                    local.get $0
                    i32.load
                    i32.load offset=12
                    call_indirect $10 (type $1)
                    i32.eqz
                    br_if $loop_1
                    br $block_3
                  end ;; $loop_1
                end ;; $block_7
                local.get $12
                i32.const 65535
                i32.gt_u
                br_if $block_5
                local.get $5
                local.get $12
                i32.const 63
                i32.and
                i32.const 128
                i32.or
                i32.store8 offset=10
                local.get $5
                local.get $12
                i32.const 6
                i32.shr_u
                i32.const 63
                i32.and
                i32.const 128
                i32.or
                i32.store8 offset=9
                local.get $5
                local.get $12
                i32.const 12
                i32.shr_u
                i32.const 15
                i32.and
                i32.const 224
                i32.or
                i32.store8 offset=8
                i32.const 3
                local.set $10
                br $block_4
              end ;; $block_6
              local.get $5
              i32.const 16
              i32.add
              global.set $12
              i32.const 0
              return
            end ;; $block_5
            local.get $5
            local.get $12
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=11
            local.get $5
            local.get $12
            i32.const 18
            i32.shr_u
            i32.const 240
            i32.or
            i32.store8 offset=8
            local.get $5
            local.get $12
            i32.const 6
            i32.shr_u
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=10
            local.get $5
            local.get $12
            i32.const 12
            i32.shr_u
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=9
            i32.const 4
            local.set $10
          end ;; $block_4
          i32.const -1
          local.set $12
          block $block_31
            loop $loop_2
              local.get $12
              i32.const 1
              i32.add
              local.tee $12
              local.get $11
              i32.ge_u
              br_if $block_31
              local.get $0
              i32.const 24
              i32.add
              i32.load
              local.get $5
              i32.const 8
              i32.add
              local.get $10
              local.get $0
              i32.const 28
              i32.add
              i32.load
              i32.load offset=12
              call_indirect $10 (type $1)
              i32.eqz
              br_if $loop_2
              br $block_3
            end ;; $loop_2
          end ;; $block_31
          local.get $0
          i32.const 24
          i32.add
          local.tee $11
          i32.load
          local.get $3
          local.get $4
          local.get $0
          i32.const 28
          i32.add
          local.tee $0
          i32.load
          i32.load offset=12
          call_indirect $10 (type $1)
          br_if $block_3
          i32.const -1
          local.set $12
          loop $loop_3
            local.get $12
            i32.const 1
            i32.add
            local.tee $12
            local.get $7
            i32.ge_u
            br_if $block_1
            local.get $11
            i32.load
            local.get $5
            i32.const 8
            i32.add
            local.get $10
            local.get $0
            i32.load
            i32.load offset=12
            call_indirect $10 (type $1)
            i32.eqz
            br_if $loop_3
          end ;; $loop_3
        end ;; $block_3
        i32.const 1
        local.set $12
      end ;; $block_2
      local.get $5
      i32.const 16
      i32.add
      global.set $12
      local.get $12
      return
    end ;; $block_1
    local.get $5
    i32.const 16
    i32.add
    global.set $12
    i32.const 0
    )
  
  (func $core::ptr::real_drop_in_place::h181e24faf7b583a9 (type $5)
    (param $0 i32)
    )
  
  (func $<T_as_core::any::Any>::get_type_id::h84858aded9b6f3c7 (type $8)
    (param $0 i32)
    (result i64)
    i64.const -1250764075464007379
    )
  
  (func $core::slice::slice_index_order_fail::h845fb1fad3fb5631 (type $0)
    (param $0 i32)
    (param $1 i32)
    (local $2 i32)
    global.get $12
    i32.const 48
    i32.sub
    local.tee $2
    global.set $12
    local.get $2
    local.get $1
    i32.store offset=4
    local.get $2
    local.get $0
    i32.store
    local.get $2
    i32.const 32
    i32.add
    i32.const 12
    i32.add
    i32.const 6
    i32.store
    local.get $2
    i32.const 28
    i32.add
    i32.const 2
    i32.store
    local.get $2
    i32.const 8
    i32.add
    i32.const 12
    i32.add
    i32.const 2
    i32.store
    local.get $2
    i32.const 6
    i32.store offset=36
    local.get $2
    i32.const 1048928
    i32.store offset=16
    local.get $2
    i32.const 2
    i32.store offset=12
    local.get $2
    i32.const 1053224
    i32.store offset=8
    local.get $2
    local.get $2
    i32.const 4
    i32.add
    i32.store offset=40
    local.get $2
    local.get $2
    i32.store offset=32
    local.get $2
    local.get $2
    i32.const 32
    i32.add
    i32.store offset=24
    local.get $2
    i32.const 8
    i32.add
    i32.const 1053240
    call $core::panicking::panic_fmt::h74ee8034b317ceed
    unreachable
    )
  
  (func $core::str::slice_error_fail::h5395f932d3594937 (type $9)
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
    global.get $12
    i32.const 112
    i32.sub
    local.tee $4
    global.set $12
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
      block $block_0
        loop $loop
          block $block_1
            local.get $8
            local.get $1
            i32.ge_u
            br_if $block_1
            local.get $0
            local.get $8
            i32.add
            i32.load8_s
            i32.const -65
            i32.gt_s
            br_if $block_0
          end ;; $block_1
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
          br $block
        end ;; $loop
      end ;; $block_0
      i32.const 0
      local.set $5
      local.get $8
      local.set $6
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
    i32.const 1049000
    i32.const 1049352
    local.get $5
    select
    i32.store offset=24
    block $block_2
      block $block_3
        block $block_4
          block $block_5
            block $block_6
              block $block_7
                block $block_8
                  block $block_9
                    block $block_10
                      local.get $2
                      local.get $1
                      i32.gt_u
                      local.tee $8
                      br_if $block_10
                      local.get $3
                      local.get $1
                      i32.gt_u
                      br_if $block_10
                      local.get $2
                      local.get $3
                      i32.gt_u
                      br_if $block_8
                      block $block_11
                        block $block_12
                          local.get $2
                          i32.eqz
                          br_if $block_12
                          local.get $1
                          local.get $2
                          i32.eq
                          br_if $block_12
                          local.get $1
                          local.get $2
                          i32.le_u
                          br_if $block_11
                          local.get $0
                          local.get $2
                          i32.add
                          i32.load8_s
                          i32.const -64
                          i32.lt_s
                          br_if $block_11
                        end ;; $block_12
                        local.get $3
                        local.set $2
                      end ;; $block_11
                      local.get $4
                      local.get $2
                      i32.store offset=32
                      local.get $2
                      i32.eqz
                      br_if $block_9
                      local.get $2
                      local.get $1
                      i32.eq
                      br_if $block_9
                      local.get $1
                      i32.const 1
                      i32.add
                      local.set $9
                      block $block_13
                        loop $loop_0
                          block $block_14
                            local.get $2
                            local.get $1
                            i32.ge_u
                            br_if $block_14
                            local.get $0
                            local.get $2
                            i32.add
                            local.tee $6
                            i32.load8_s
                            i32.const -65
                            i32.gt_s
                            br_if $block_13
                          end ;; $block_14
                          local.get $2
                          i32.const -1
                          i32.add
                          local.set $8
                          block $block_15
                            local.get $2
                            i32.const 1
                            i32.eq
                            br_if $block_15
                            local.get $9
                            local.get $2
                            i32.eq
                            local.set $6
                            local.get $8
                            local.set $2
                            local.get $6
                            i32.eqz
                            br_if $loop_0
                          end ;; $block_15
                        end ;; $loop_0
                        local.get $0
                        local.get $8
                        i32.add
                        local.tee $6
                        local.get $0
                        local.get $1
                        i32.add
                        local.tee $2
                        i32.ne
                        br_if $block_7
                        br $block_6
                      end ;; $block_13
                      local.get $2
                      local.set $8
                      local.get $6
                      local.get $0
                      local.get $1
                      i32.add
                      local.tee $2
                      i32.eq
                      br_if $block_6
                      br $block_7
                    end ;; $block_10
                    local.get $4
                    local.get $2
                    local.get $3
                    local.get $8
                    select
                    i32.store offset=40
                    local.get $4
                    i32.const 72
                    i32.add
                    i32.const 20
                    i32.add
                    i32.const 1
                    i32.store
                    local.get $4
                    i32.const 72
                    i32.add
                    i32.const 12
                    i32.add
                    i32.const 1
                    i32.store
                    local.get $4
                    i32.const 48
                    i32.add
                    i32.const 20
                    i32.add
                    i32.const 3
                    i32.store
                    local.get $4
                    i32.const 48
                    i32.add
                    i32.const 12
                    i32.add
                    i32.const 3
                    i32.store
                    local.get $4
                    i32.const 6
                    i32.store offset=76
                    local.get $4
                    i32.const 1049360
                    i32.store offset=56
                    local.get $4
                    i32.const 3
                    i32.store offset=52
                    local.get $4
                    i32.const 1053256
                    i32.store offset=48
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
                    local.get $4
                    i32.const 72
                    i32.add
                    i32.store offset=64
                    local.get $4
                    i32.const 48
                    i32.add
                    i32.const 1053280
                    call $core::panicking::panic_fmt::h74ee8034b317ceed
                    unreachable
                  end ;; $block_9
                  local.get $0
                  local.get $2
                  local.tee $8
                  i32.add
                  local.tee $6
                  local.get $0
                  local.get $1
                  i32.add
                  local.tee $2
                  i32.ne
                  br_if $block_7
                  br $block_6
                end ;; $block_8
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
                i32.const 72
                i32.add
                i32.const 12
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
                i32.const 48
                i32.add
                i32.const 12
                i32.add
                i32.const 4
                i32.store
                local.get $4
                i32.const 6
                i32.store offset=76
                local.get $4
                i32.const 1049468
                i32.store offset=56
                local.get $4
                i32.const 4
                i32.store offset=52
                local.get $4
                i32.const 1053296
                i32.store offset=48
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
                local.get $4
                i32.const 72
                i32.add
                i32.store offset=64
                local.get $4
                i32.const 48
                i32.add
                i32.const 1053328
                call $core::panicking::panic_fmt::h74ee8034b317ceed
                unreachable
              end ;; $block_7
              i32.const 1
              local.set $1
              i32.const 0
              local.set $9
              block $block_16
                local.get $6
                i32.load8_s
                local.tee $6
                i32.const 0
                i32.lt_s
                br_if $block_16
                local.get $4
                local.get $6
                i32.const 255
                i32.and
                i32.store offset=36
                local.get $4
                i32.const 40
                i32.add
                local.set $2
                br $block_2
              end ;; $block_16
              local.get $2
              local.set $1
              block $block_17
                local.get $0
                local.get $8
                i32.add
                local.tee $0
                i32.const 1
                i32.add
                local.get $2
                i32.eq
                br_if $block_17
                local.get $0
                i32.const 2
                i32.add
                local.set $1
                local.get $0
                i32.const 1
                i32.add
                i32.load8_u
                i32.const 63
                i32.and
                local.set $9
              end ;; $block_17
              local.get $6
              i32.const 31
              i32.and
              local.set $0
              local.get $6
              i32.const 255
              i32.and
              i32.const 224
              i32.lt_u
              br_if $block_5
              i32.const 0
              local.set $5
              local.get $2
              local.set $7
              block $block_18
                local.get $1
                local.get $2
                i32.eq
                br_if $block_18
                local.get $1
                i32.const 1
                i32.add
                local.set $7
                local.get $1
                i32.load8_u
                i32.const 63
                i32.and
                local.set $5
              end ;; $block_18
              local.get $5
              local.get $9
              i32.const 6
              i32.shl
              i32.or
              local.set $1
              local.get $6
              i32.const 255
              i32.and
              i32.const 240
              i32.lt_u
              br_if $block_4
              i32.const 0
              local.set $6
              block $block_19
                local.get $7
                local.get $2
                i32.eq
                br_if $block_19
                local.get $7
                i32.load8_u
                i32.const 63
                i32.and
                local.set $6
              end ;; $block_19
              local.get $1
              i32.const 6
              i32.shl
              local.get $0
              i32.const 18
              i32.shl
              i32.const 1835008
              i32.and
              i32.or
              local.get $6
              i32.or
              local.tee $6
              i32.const 1114112
              i32.ne
              br_if $block_3
            end ;; $block_6
            i32.const 1053344
            call $core::panicking::panic::h13421bef49a1c627
            unreachable
          end ;; $block_5
          local.get $9
          local.get $0
          i32.const 6
          i32.shl
          i32.or
          local.set $6
          br $block_3
        end ;; $block_4
        local.get $1
        local.get $0
        i32.const 12
        i32.shl
        i32.or
        local.set $6
      end ;; $block_3
      local.get $4
      local.get $6
      i32.store offset=36
      i32.const 1
      local.set $1
      local.get $4
      i32.const 40
      i32.add
      local.set $2
      local.get $6
      i32.const 128
      i32.lt_u
      br_if $block_2
      i32.const 2
      local.set $1
      local.get $6
      i32.const 2048
      i32.lt_u
      br_if $block_2
      i32.const 3
      i32.const 4
      local.get $6
      i32.const 65536
      i32.lt_u
      select
      local.set $1
    end ;; $block_2
    local.get $4
    local.get $8
    i32.store offset=40
    local.get $4
    local.get $1
    local.get $8
    i32.add
    i32.store offset=44
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
    i32.const 72
    i32.add
    i32.const 12
    i32.add
    i32.const 8
    i32.store
    local.get $4
    i32.const 48
    i32.add
    i32.const 20
    i32.add
    i32.const 5
    i32.store
    local.get $4
    i32.const 48
    i32.add
    i32.const 12
    i32.add
    i32.const 5
    i32.store
    local.get $4
    local.get $2
    i32.store offset=88
    local.get $4
    i32.const 6
    i32.store offset=76
    local.get $4
    i32.const 1049612
    i32.store offset=56
    local.get $4
    i32.const 5
    i32.store offset=52
    local.get $4
    i32.const 1053368
    i32.store offset=48
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
    local.get $4
    i32.const 72
    i32.add
    i32.store offset=64
    local.get $4
    i32.const 48
    i32.add
    i32.const 1053408
    call $core::panicking::panic_fmt::h74ee8034b317ceed
    unreachable
    )
  
  (func $<core::ops::range::Range<Idx>_as_core::fmt::Debug>::fmt::hd43772a3c2cd5a20 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    global.get $12
    i32.const 48
    i32.sub
    local.tee $2
    global.set $12
    local.get $2
    i32.const 8
    i32.add
    i32.const 12
    i32.add
    i32.const 9
    i32.store
    local.get $2
    i32.const 9
    i32.store offset=12
    local.get $2
    local.get $0
    i32.store offset=8
    local.get $2
    local.get $0
    i32.const 4
    i32.add
    i32.store offset=16
    local.get $1
    i32.const 28
    i32.add
    i32.load
    local.set $0
    local.get $1
    i32.load offset=24
    local.set $1
    local.get $2
    i32.const 44
    i32.add
    i32.const 2
    i32.store
    local.get $2
    i32.const 24
    i32.add
    i32.const 12
    i32.add
    i32.const 2
    i32.store
    local.get $2
    i32.const 1048928
    i32.store offset=32
    local.get $2
    i32.const 2
    i32.store offset=28
    local.get $2
    i32.const 1053424
    i32.store offset=24
    local.get $2
    local.get $2
    i32.const 8
    i32.add
    i32.store offset=40
    local.get $1
    local.get $0
    local.get $2
    i32.const 24
    i32.add
    call $core::fmt::write::h2bc7cdeb5edd0d71
    local.set $1
    local.get $2
    i32.const 48
    i32.add
    global.set $12
    local.get $1
    )
  
  (func $<char_as_core::fmt::Debug>::fmt::hda62120a64941310 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    (local $5 i64)
    (local $6 i32)
    (local $7 i32)
    (local $8 i32)
    (local $9 i32)
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
      call_indirect $10 (type $2)
      br_if $block
      i32.const 2
      local.set $2
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
                          block $block_10
                            block $block_11
                              block $block_12
                                block $block_13
                                  block $block_14
                                    block $block_15
                                      block $block_16
                                        block $block_17
                                          local.get $0
                                          i32.load
                                          local.tee $0
                                          i32.const -9
                                          i32.add
                                          local.tee $3
                                          i32.const 30
                                          i32.gt_u
                                          br_if $block_17
                                          i32.const 116
                                          local.set $4
                                          block $block_18
                                            local.get $3
                                            br_table
                                              $block_0 $block_18 $block_16 $block_16 $block_15 $block_16 $block_16 $block_16 $block_16 $block_16 $block_16 $block_16 $block_16 $block_16 $block_16 $block_16
                                              $block_16 $block_16 $block_16 $block_16 $block_16 $block_16 $block_16 $block_16 $block_16 $block_2 $block_16 $block_16 $block_16 $block_16 $block_2
                                              $block_0 ;; default
                                          end ;; $block_18
                                          i32.const 110
                                          local.set $4
                                          br $block_14
                                        end ;; $block_17
                                        local.get $0
                                        i32.const 92
                                        i32.eq
                                        br_if $block_2
                                      end ;; $block_16
                                      local.get $0
                                      i32.const 2048
                                      i32.ge_u
                                      br_if $block_13
                                      local.get $0
                                      i32.const 3
                                      i32.shr_u
                                      i32.const 262136
                                      i32.and
                                      i32.const 1053472
                                      i32.add
                                      local.set $2
                                      br $block_12
                                    end ;; $block_15
                                    i32.const 114
                                    local.set $4
                                  end ;; $block_14
                                  br $block_0
                                end ;; $block_13
                                block $block_19
                                  local.get $0
                                  i32.const 65536
                                  i32.ge_u
                                  br_if $block_19
                                  local.get $0
                                  i32.const 6
                                  i32.shr_u
                                  i32.const -32
                                  i32.add
                                  local.tee $2
                                  i32.const 992
                                  i32.ge_u
                                  br_if $block_11
                                  local.get $2
                                  i32.const 1053752
                                  i32.add
                                  i32.load8_u
                                  local.tee $2
                                  i32.const 73
                                  i32.gt_u
                                  br_if $block_10
                                  local.get $2
                                  i32.const 3
                                  i32.shl
                                  i32.const 1051248
                                  i32.add
                                  local.set $2
                                  br $block_12
                                end ;; $block_19
                                local.get $0
                                i32.const 12
                                i32.shr_u
                                i32.const -16
                                i32.add
                                local.tee $2
                                i32.const 256
                                i32.ge_u
                                br_if $block_9
                                local.get $2
                                i32.const 1054744
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
                                local.tee $2
                                i32.const 511
                                i32.gt_u
                                br_if $block_8
                                local.get $2
                                i32.const 1051840
                                i32.add
                                i32.load8_u
                                local.tee $2
                                i32.const 54
                                i32.gt_u
                                br_if $block_7
                                local.get $2
                                i32.const 3
                                i32.shl
                                i32.const 1052352
                                i32.add
                                local.set $2
                              end ;; $block_12
                              block $block_20
                                block $block_21
                                  block $block_22
                                    block $block_23
                                      local.get $2
                                      i64.load
                                      i64.const 1
                                      local.get $0
                                      i32.const 63
                                      i32.and
                                      i64.extend_i32_u
                                      i64.shl
                                      i64.and
                                      i64.eqz
                                      br_if $block_23
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
                                      local.set $5
                                      br $block_22
                                    end ;; $block_23
                                    block $block_24
                                      block $block_25
                                        local.get $0
                                        i32.const 65535
                                        i32.gt_u
                                        br_if $block_25
                                        local.get $0
                                        i32.const 65280
                                        i32.and
                                        i32.const 8
                                        i32.shr_u
                                        local.set $6
                                        i32.const 1049912
                                        local.set $3
                                        i32.const 0
                                        local.set $7
                                        local.get $0
                                        i32.const 255
                                        i32.and
                                        local.set $8
                                        block $block_26
                                          loop $loop
                                            local.get $3
                                            i32.const 2
                                            i32.add
                                            local.set $9
                                            local.get $7
                                            local.get $3
                                            i32.load8_u offset=1
                                            local.tee $2
                                            i32.add
                                            local.set $4
                                            block $block_27
                                              block $block_28
                                                local.get $3
                                                i32.load8_u
                                                local.tee $3
                                                local.get $6
                                                i32.ne
                                                br_if $block_28
                                                local.get $4
                                                local.get $7
                                                i32.lt_u
                                                br_if $block_21
                                                local.get $4
                                                i32.const 304
                                                i32.ge_u
                                                br_if $block_20
                                                local.get $7
                                                i32.const 1049992
                                                i32.add
                                                local.set $3
                                                loop $loop_0
                                                  local.get $2
                                                  i32.eqz
                                                  br_if $block_27
                                                  local.get $2
                                                  i32.const -1
                                                  i32.add
                                                  local.set $2
                                                  local.get $3
                                                  i32.load8_u
                                                  local.set $7
                                                  local.get $3
                                                  i32.const 1
                                                  i32.add
                                                  local.set $3
                                                  local.get $7
                                                  local.get $8
                                                  i32.ne
                                                  br_if $loop_0
                                                  br $block_24
                                                end ;; $loop_0
                                              end ;; $block_28
                                              local.get $3
                                              local.get $6
                                              i32.gt_u
                                              br_if $block_26
                                              local.get $4
                                              local.set $7
                                              local.get $9
                                              local.set $3
                                              local.get $9
                                              i32.const 1049992
                                              i32.ne
                                              br_if $loop
                                              br $block_26
                                            end ;; $block_27
                                            local.get $4
                                            local.set $7
                                            local.get $9
                                            local.set $3
                                            local.get $9
                                            i32.const 1049992
                                            i32.ne
                                            br_if $loop
                                          end ;; $loop
                                        end ;; $block_26
                                        local.get $0
                                        i32.const 65535
                                        i32.and
                                        local.set $3
                                        i32.const 1050295
                                        local.set $2
                                        i32.const 1
                                        local.set $7
                                        block $block_29
                                          loop $loop_1
                                            local.get $2
                                            i32.const 1
                                            i32.add
                                            local.set $8
                                            block $block_30
                                              block $block_31
                                                local.get $2
                                                i32.load8_u
                                                local.tee $4
                                                i32.const 24
                                                i32.shl
                                                i32.const 24
                                                i32.shr_s
                                                local.tee $9
                                                i32.const -1
                                                i32.le_s
                                                br_if $block_31
                                                local.get $8
                                                local.set $2
                                                local.get $3
                                                local.get $4
                                                i32.sub
                                                local.tee $3
                                                i32.const 0
                                                i32.ge_s
                                                br_if $block_30
                                                br $block_29
                                              end ;; $block_31
                                              local.get $8
                                              i32.const 1050611
                                              i32.eq
                                              br_if $block_4
                                              local.get $2
                                              i32.const 1
                                              i32.add
                                              local.set $8
                                              local.get $2
                                              i32.const 2
                                              i32.add
                                              local.set $2
                                              local.get $3
                                              local.get $9
                                              i32.const 127
                                              i32.and
                                              i32.const 8
                                              i32.shl
                                              local.get $8
                                              i32.load8_u
                                              i32.or
                                              i32.sub
                                              local.tee $3
                                              i32.const 0
                                              i32.lt_s
                                              br_if $block_29
                                            end ;; $block_30
                                            local.get $7
                                            i32.const 1
                                            i32.xor
                                            local.set $7
                                            local.get $2
                                            i32.const 1050611
                                            i32.ne
                                            br_if $loop_1
                                          end ;; $loop_1
                                        end ;; $block_29
                                        i32.const 1
                                        local.set $2
                                        local.get $7
                                        i32.const 1
                                        i32.and
                                        i32.eqz
                                        br_if $block_24
                                        br $block_2
                                      end ;; $block_25
                                      block $block_32
                                        local.get $0
                                        i32.const 131071
                                        i32.gt_u
                                        br_if $block_32
                                        local.get $0
                                        i32.const 65280
                                        i32.and
                                        i32.const 8
                                        i32.shr_u
                                        local.set $6
                                        i32.const 1050611
                                        local.set $3
                                        i32.const 0
                                        local.set $7
                                        local.get $0
                                        i32.const 255
                                        i32.and
                                        local.set $8
                                        block $block_33
                                          loop $loop_2
                                            local.get $3
                                            i32.const 2
                                            i32.add
                                            local.set $9
                                            local.get $7
                                            local.get $3
                                            i32.load8_u offset=1
                                            local.tee $2
                                            i32.add
                                            local.set $4
                                            block $block_34
                                              block $block_35
                                                local.get $3
                                                i32.load8_u
                                                local.tee $3
                                                local.get $6
                                                i32.ne
                                                br_if $block_35
                                                local.get $4
                                                local.get $7
                                                i32.lt_u
                                                br_if $block_6
                                                local.get $4
                                                i32.const 159
                                                i32.ge_u
                                                br_if $block_5
                                                local.get $7
                                                i32.const 1050677
                                                i32.add
                                                local.set $3
                                                loop $loop_3
                                                  local.get $2
                                                  i32.eqz
                                                  br_if $block_34
                                                  local.get $2
                                                  i32.const -1
                                                  i32.add
                                                  local.set $2
                                                  local.get $3
                                                  i32.load8_u
                                                  local.set $7
                                                  local.get $3
                                                  i32.const 1
                                                  i32.add
                                                  local.set $3
                                                  local.get $7
                                                  local.get $8
                                                  i32.ne
                                                  br_if $loop_3
                                                  br $block_24
                                                end ;; $loop_3
                                              end ;; $block_35
                                              local.get $3
                                              local.get $6
                                              i32.gt_u
                                              br_if $block_33
                                              local.get $4
                                              local.set $7
                                              local.get $9
                                              local.set $3
                                              local.get $9
                                              i32.const 1050677
                                              i32.ne
                                              br_if $loop_2
                                              br $block_33
                                            end ;; $block_34
                                            local.get $4
                                            local.set $7
                                            local.get $9
                                            local.set $3
                                            local.get $9
                                            i32.const 1050677
                                            i32.ne
                                            br_if $loop_2
                                          end ;; $loop_2
                                        end ;; $block_33
                                        local.get $0
                                        i32.const 65535
                                        i32.and
                                        local.set $3
                                        i32.const 1050835
                                        local.set $2
                                        i32.const 1
                                        local.set $7
                                        block $block_36
                                          loop $loop_4
                                            local.get $2
                                            i32.const 1
                                            i32.add
                                            local.set $8
                                            block $block_37
                                              block $block_38
                                                local.get $2
                                                i32.load8_u
                                                local.tee $4
                                                i32.const 24
                                                i32.shl
                                                i32.const 24
                                                i32.shr_s
                                                local.tee $9
                                                i32.const -1
                                                i32.le_s
                                                br_if $block_38
                                                local.get $8
                                                local.set $2
                                                local.get $3
                                                local.get $4
                                                i32.sub
                                                local.tee $3
                                                i32.const 0
                                                i32.ge_s
                                                br_if $block_37
                                                br $block_36
                                              end ;; $block_38
                                              local.get $8
                                              i32.const 1051216
                                              i32.eq
                                              br_if $block_3
                                              local.get $2
                                              i32.const 1
                                              i32.add
                                              local.set $8
                                              local.get $2
                                              i32.const 2
                                              i32.add
                                              local.set $2
                                              local.get $3
                                              local.get $9
                                              i32.const 127
                                              i32.and
                                              i32.const 8
                                              i32.shl
                                              local.get $8
                                              i32.load8_u
                                              i32.or
                                              i32.sub
                                              local.tee $3
                                              i32.const 0
                                              i32.lt_s
                                              br_if $block_36
                                            end ;; $block_37
                                            local.get $7
                                            i32.const 1
                                            i32.xor
                                            local.set $7
                                            local.get $2
                                            i32.const 1051216
                                            i32.ne
                                            br_if $loop_4
                                          end ;; $loop_4
                                        end ;; $block_36
                                        i32.const 1
                                        local.set $2
                                        local.get $7
                                        i32.const 1
                                        i32.and
                                        i32.eqz
                                        br_if $block_24
                                        br $block_2
                                      end ;; $block_32
                                      local.get $0
                                      i32.const 917999
                                      i32.gt_u
                                      br_if $block_24
                                      local.get $0
                                      i32.const -195102
                                      i32.add
                                      i32.const 722658
                                      i32.lt_u
                                      br_if $block_24
                                      local.get $0
                                      i32.const -191457
                                      i32.add
                                      i32.const 3103
                                      i32.lt_u
                                      br_if $block_24
                                      local.get $0
                                      i32.const -183970
                                      i32.add
                                      i32.const 14
                                      i32.lt_u
                                      br_if $block_24
                                      local.get $0
                                      i32.const 2097150
                                      i32.and
                                      i32.const 178206
                                      i32.eq
                                      br_if $block_24
                                      local.get $0
                                      i32.const -173783
                                      i32.add
                                      i32.const 41
                                      i32.lt_u
                                      br_if $block_24
                                      local.get $0
                                      i32.const -177973
                                      i32.add
                                      i32.const 10
                                      i32.le_u
                                      br_if $block_24
                                      i32.const 1
                                      local.set $2
                                      br $block_2
                                    end ;; $block_24
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
                                    local.set $5
                                  end ;; $block_22
                                  i32.const 3
                                  local.set $2
                                  br $block_1
                                end ;; $block_21
                                local.get $7
                                local.get $4
                                call $core::slice::slice_index_order_fail::h845fb1fad3fb5631
                                unreachable
                              end ;; $block_20
                              local.get $4
                              i32.const 303
                              call $core::slice::slice_index_len_fail::h7a4ef59ca849a060
                              unreachable
                            end ;; $block_11
                            i32.const 1055000
                            local.get $2
                            i32.const 992
                            call $core::panicking::panic_bounds_check::h34b80e64d41db052
                            unreachable
                          end ;; $block_10
                          i32.const 1055016
                          local.get $2
                          i32.const 74
                          call $core::panicking::panic_bounds_check::h34b80e64d41db052
                          unreachable
                        end ;; $block_9
                        i32.const 1055032
                        local.get $2
                        i32.const 256
                        call $core::panicking::panic_bounds_check::h34b80e64d41db052
                        unreachable
                      end ;; $block_8
                      i32.const 1055048
                      local.get $2
                      i32.const 512
                      call $core::panicking::panic_bounds_check::h34b80e64d41db052
                      unreachable
                    end ;; $block_7
                    i32.const 1055064
                    local.get $2
                    i32.const 55
                    call $core::panicking::panic_bounds_check::h34b80e64d41db052
                    unreachable
                  end ;; $block_6
                  local.get $7
                  local.get $4
                  call $core::slice::slice_index_order_fail::h845fb1fad3fb5631
                  unreachable
                end ;; $block_5
                local.get $4
                i32.const 158
                call $core::slice::slice_index_len_fail::h7a4ef59ca849a060
                unreachable
              end ;; $block_4
              i32.const 1053344
              call $core::panicking::panic::h13421bef49a1c627
              unreachable
            end ;; $block_3
            i32.const 1053344
            call $core::panicking::panic::h13421bef49a1c627
            unreachable
          end ;; $block_2
        end ;; $block_1
        local.get $0
        local.set $4
      end ;; $block_0
      local.get $5
      i64.const 32
      i64.shr_u
      i32.wrap_i64
      local.set $8
      local.get $5
      i32.wrap_i64
      local.set $9
      local.get $1
      i32.const 24
      i32.add
      local.set $3
      local.get $1
      i32.const 28
      i32.add
      local.set $7
      loop $loop_5
        block $block_39
          block $block_40
            block $block_41
              block $block_42
                block $block_43
                  block $block_44
                    local.get $2
                    i32.const 1
                    i32.eq
                    br_if $block_44
                    i32.const 92
                    local.set $0
                    block $block_45
                      local.get $2
                      i32.const 2
                      i32.eq
                      br_if $block_45
                      local.get $2
                      i32.const 3
                      i32.ne
                      br_if $block_40
                      local.get $8
                      i32.const 255
                      i32.and
                      local.set $2
                      i32.const 4
                      local.set $8
                      local.get $2
                      i32.const -1
                      i32.add
                      local.tee $6
                      i32.const 4
                      i32.gt_u
                      br_if $block_40
                      i32.const 3
                      local.set $2
                      block $block_46
                        local.get $6
                        br_table
                          $block_46 $block_42 $block_41 $block_43 $block_39
                          $block_46 ;; default
                      end ;; $block_46
                      i32.const 0
                      local.set $8
                      i32.const 125
                      local.set $0
                      br $block_39
                    end ;; $block_45
                    i32.const 1
                    local.set $2
                    br $block_39
                  end ;; $block_44
                  i32.const 0
                  local.set $2
                  local.get $4
                  local.set $0
                  br $block_39
                end ;; $block_43
                i32.const 3
                local.set $2
                i32.const 117
                local.set $0
                i32.const 3
                local.set $8
                br $block_39
              end ;; $block_42
              local.get $4
              local.get $9
              i32.const 2
              i32.shl
              i32.const 28
              i32.and
              i32.shr_u
              i32.const 15
              i32.and
              local.tee $0
              i32.const 48
              i32.or
              local.get $0
              i32.const 87
              i32.add
              local.get $0
              i32.const 10
              i32.lt_u
              select
              local.set $0
              i32.const 2
              i32.const 1
              local.get $9
              select
              local.set $8
              local.get $9
              i32.const -1
              i32.add
              i32.const 0
              local.get $9
              select
              local.set $9
              br $block_39
            end ;; $block_41
            i32.const 2
            local.set $8
            i32.const 123
            local.set $0
            br $block_39
          end ;; $block_40
          local.get $1
          i32.const 24
          i32.add
          i32.load
          i32.const 39
          local.get $1
          i32.const 28
          i32.add
          i32.load
          i32.load offset=16
          call_indirect $10 (type $2)
          local.set $2
          br $block
        end ;; $block_39
        local.get $3
        i32.load
        local.get $0
        local.get $7
        i32.load
        i32.load offset=16
        call_indirect $10 (type $2)
        i32.eqz
        br_if $loop_5
      end ;; $loop_5
      i32.const 1
      return
    end ;; $block
    local.get $2
    )
  
  (func $core::fmt::num::<impl_core::fmt::Debug_for_usize>::fmt::h2a5aa117bff2c084 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    (local $3 i32)
    (local $4 i32)
    global.get $12
    i32.const 128
    i32.sub
    local.tee $2
    global.set $12
    block $block
      block $block_0
        block $block_1
          block $block_2
            local.get $1
            i32.load
            local.tee $3
            i32.const 16
            i32.and
            br_if $block_2
            local.get $3
            i32.const 32
            i32.and
            br_if $block_1
            local.get $0
            local.get $1
            call $core::fmt::num::<impl_core::fmt::Display_for_u32>::fmt::h4f4d56a15c6233ed
            local.set $0
            local.get $2
            i32.const 128
            i32.add
            global.set $12
            local.get $0
            return
          end ;; $block_2
          local.get $0
          i32.load
          local.set $3
          i32.const 0
          local.set $0
          loop $loop
            local.get $2
            local.get $0
            i32.add
            i32.const 127
            i32.add
            local.get $3
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
            local.get $0
            i32.const -1
            i32.add
            local.set $0
            local.get $3
            i32.const 4
            i32.shr_u
            local.tee $3
            br_if $loop
          end ;; $loop
          local.get $0
          i32.const 128
          i32.add
          local.tee $3
          i32.const 129
          i32.ge_u
          br_if $block_0
          local.get $1
          i32.const 1049350
          i32.const 2
          local.get $2
          local.get $0
          i32.add
          i32.const 128
          i32.add
          i32.const 0
          local.get $0
          i32.sub
          call $core::fmt::Formatter::pad_integral::hb6042588a2a3edea
          local.set $0
          local.get $2
          i32.const 128
          i32.add
          global.set $12
          local.get $0
          return
        end ;; $block_1
        local.get $0
        i32.load
        local.set $3
        i32.const 0
        local.set $0
        loop $loop_0
          local.get $2
          local.get $0
          i32.add
          i32.const 127
          i32.add
          local.get $3
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
          local.get $0
          i32.const -1
          i32.add
          local.set $0
          local.get $3
          i32.const 4
          i32.shr_u
          local.tee $3
          br_if $loop_0
        end ;; $loop_0
        local.get $0
        i32.const 128
        i32.add
        local.tee $3
        i32.const 129
        i32.ge_u
        br_if $block
        local.get $1
        i32.const 1049350
        i32.const 2
        local.get $2
        local.get $0
        i32.add
        i32.const 128
        i32.add
        i32.const 0
        local.get $0
        i32.sub
        call $core::fmt::Formatter::pad_integral::hb6042588a2a3edea
        local.set $0
        local.get $2
        i32.const 128
        i32.add
        global.set $12
        local.get $0
        return
      end ;; $block_0
      local.get $3
      i32.const 128
      call $core::slice::slice_index_order_fail::h845fb1fad3fb5631
      unreachable
    end ;; $block
    local.get $3
    i32.const 128
    call $core::slice::slice_index_order_fail::h845fb1fad3fb5631
    unreachable
    )
  
  (func $core::fmt::ArgumentV1::show_usize::h7e76c008016dd45f (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    local.get $0
    local.get $1
    call $core::fmt::num::<impl_core::fmt::Display_for_u32>::fmt::h4f4d56a15c6233ed
    )
  
  (func $core::slice::memchr::memchr::h2712233316cf809b (type $9)
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
      local.get $2
      i32.const 3
      i32.and
      local.tee $5
      i32.eqz
      br_if $block
      i32.const 4
      local.get $5
      i32.sub
      local.tee $5
      i32.eqz
      br_if $block
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
      block $block_0
        block $block_1
          loop $loop
            local.get $6
            local.get $9
            i32.sub
            i32.const 3
            i32.le_u
            br_if $block_1
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
            br_if $block_0
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
            br_if $block_0
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
            br_if $block_0
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
            br $block_0
          end ;; $loop
        end ;; $block_1
        i32.const 0
        local.set $7
        local.get $1
        i32.const 255
        i32.and
        local.set $6
        loop $loop_0
          local.get $8
          i32.eqz
          br_if $block
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
      end ;; $block_0
      local.get $0
      local.get $5
      i32.store offset=4
      local.get $0
      i32.const 1
      i32.store
      return
    end ;; $block
    local.get $1
    i32.const 255
    i32.and
    local.set $7
    block $block_2
      block $block_3
        local.get $3
        i32.const 8
        i32.lt_u
        br_if $block_3
        local.get $4
        local.get $3
        i32.const -8
        i32.add
        local.tee $10
        i32.gt_u
        br_if $block_3
        local.get $7
        i32.const 16843009
        i32.mul
        local.set $5
        block $block_4
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
            br_if $block_4
            local.get $4
            i32.const 8
            i32.add
            local.tee $4
            local.get $10
            i32.le_u
            br_if $loop_1
          end ;; $loop_1
        end ;; $block_4
        local.get $4
        local.get $3
        i32.gt_u
        br_if $block_2
      end ;; $block_3
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
      block $block_5
        block $block_6
          block $block_7
            loop $loop_2
              local.get $2
              local.get $9
              i32.sub
              i32.const 3
              i32.le_u
              br_if $block_7
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
              br_if $block_6
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
              br_if $block_6
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
              br_if $block_6
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
              br $block_6
            end ;; $loop_2
          end ;; $block_7
          i32.const 0
          local.set $7
          local.get $1
          i32.const 255
          i32.and
          local.set $2
          loop $loop_3
            local.get $8
            i32.eqz
            br_if $block_5
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
        end ;; $block_6
        local.get $0
        local.get $5
        local.get $4
        i32.add
        i32.store offset=4
        local.get $0
        i32.const 1
        i32.store
        return
      end ;; $block_5
      local.get $0
      local.get $5
      local.get $7
      i32.add
      local.get $4
      i32.add
      i32.store offset=4
      local.get $0
      i32.const 0
      i32.store
      return
    end ;; $block_2
    local.get $4
    local.get $3
    call $core::slice::slice_index_order_fail::h845fb1fad3fb5631
    unreachable
    )
  
  (func $core::ptr::real_drop_in_place::h7a6e9b3a26682e2c (type $5)
    (param $0 i32)
    )
  
  (func $core::fmt::Write::write_char::h699f698034020179 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    global.get $12
    i32.const 16
    i32.sub
    local.tee $2
    global.set $12
    local.get $2
    i32.const 0
    i32.store offset=12
    block $block
      block $block_0
        local.get $1
        i32.const 127
        i32.gt_u
        br_if $block_0
        local.get $2
        local.get $1
        i32.store8 offset=12
        i32.const 1
        local.set $1
        br $block
      end ;; $block_0
      block $block_1
        local.get $1
        i32.const 2047
        i32.gt_u
        br_if $block_1
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
        br $block
      end ;; $block_1
      block $block_2
        local.get $1
        i32.const 65535
        i32.gt_u
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
    end ;; $block
    local.get $0
    local.get $2
    i32.const 12
    i32.add
    local.get $1
    call $<core::fmt::builders::PadAdapter<'_>_as_core::fmt::Write>::write_str::heb6658a92b0d254f
    local.set $1
    local.get $2
    i32.const 16
    i32.add
    global.set $12
    local.get $1
    )
  
  (func $core::fmt::Write::write_fmt::h88700fc7b71e07ba (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    global.get $12
    i32.const 32
    i32.sub
    local.tee $2
    global.set $12
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
    i32.const 1055104
    local.get $2
    i32.const 8
    i32.add
    call $core::fmt::write::h2bc7cdeb5edd0d71
    local.set $1
    local.get $2
    i32.const 32
    i32.add
    global.set $12
    local.get $1
    )
  
  (func $core::ptr::real_drop_in_place::h0e270dec7857294a (type $5)
    (param $0 i32)
    )
  
  (func $<&mut_W_as_core::fmt::Write>::write_str::h5e939bfff6b7262a (type $1)
    (param $0 i32)
    (param $1 i32)
    (param $2 i32)
    (result i32)
    local.get $0
    i32.load
    local.get $1
    local.get $2
    call $<core::fmt::builders::PadAdapter<'_>_as_core::fmt::Write>::write_str::heb6658a92b0d254f
    )
  
  (func $<&mut_W_as_core::fmt::Write>::write_char::h6c366a2088812c15 (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    global.get $12
    i32.const 16
    i32.sub
    local.tee $2
    global.set $12
    local.get $0
    i32.load
    local.set $0
    local.get $2
    i32.const 0
    i32.store offset=12
    block $block
      block $block_0
        local.get $1
        i32.const 127
        i32.gt_u
        br_if $block_0
        local.get $2
        local.get $1
        i32.store8 offset=12
        i32.const 1
        local.set $1
        br $block
      end ;; $block_0
      block $block_1
        local.get $1
        i32.const 2047
        i32.gt_u
        br_if $block_1
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
        br $block
      end ;; $block_1
      block $block_2
        local.get $1
        i32.const 65535
        i32.gt_u
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
    end ;; $block
    local.get $0
    local.get $2
    i32.const 12
    i32.add
    local.get $1
    call $<core::fmt::builders::PadAdapter<'_>_as_core::fmt::Write>::write_str::heb6658a92b0d254f
    local.set $1
    local.get $2
    i32.const 16
    i32.add
    global.set $12
    local.get $1
    )
  
  (func $<&mut_W_as_core::fmt::Write>::write_fmt::h1bb28d0d3f773a6a (type $2)
    (param $0 i32)
    (param $1 i32)
    (result i32)
    (local $2 i32)
    global.get $12
    i32.const 32
    i32.sub
    local.tee $2
    global.set $12
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
    i32.const 1055104
    local.get $2
    i32.const 8
    i32.add
    call $core::fmt::write::h2bc7cdeb5edd0d71
    local.set $1
    local.get $2
    i32.const 32
    i32.add
    global.set $12
    local.get $1
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
    block $block
      block $block_0
        local.get $2
        i32.eqz
        br_if $block_0
        i32.const 0
        local.set $3
        loop $loop
          local.get $0
          local.get $3
          i32.add
          i32.load8_u
          local.tee $4
          local.get $1
          local.get $3
          i32.add
          i32.load8_u
          local.tee $5
          i32.ne
          br_if $block
          local.get $3
          i32.const 1
          i32.add
          local.tee $3
          local.get $2
          i32.lt_u
          br_if $loop
        end ;; $loop
        i32.const 0
        return
      end ;; $block_0
      i32.const 0
      return
    end ;; $block
    local.get $4
    local.get $5
    i32.sub
    )
  ;; User section "producers":
    ;; "\02\08language\01\04Rust\042018\0cprocessed-"
    ;; "by\01\05rustc\1d1.33.0 (2aa4c46cf 2019"
    ;; "-02-28)"
  )