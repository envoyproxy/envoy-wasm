# Experimental Golang SDK

To generate a test Wasm file with the given sample:
1. Download [tinygo docker image]( https://tinygo.org/getting-started/using-docker/).
2. Run `cd api/wasm/go && docker run -it --rm -v $(pwd):/src tinygo/tinygo:0.8.0 bash`.
3. Copy `sdk` into go/src `cd src/ && cp -r envoywasmsdk/ /usr/local/go/src/`.
4. Run `tinygo build -target wasm -o filter.wasm -heap-size 10M -wasm-abi generic sample/filter.go`.