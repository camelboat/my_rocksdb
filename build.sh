make rocksdbjavastaticrelease -j
cp java/target/rocksdbjni-6.14.0.jar ~/YCSB/rocksdb/target/dependency/
rm ~/YCSB/rocksdb/target/dependency/rocksdbjni-6.2.2.jar
