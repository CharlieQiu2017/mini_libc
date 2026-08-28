#!/bin/sh

head -c 32 /dev/urandom > sk.bin
./gen_key < sk.bin > pk.bin
head -c 48 /dev/urandom > seed_salt.bin
head -c 32 /dev/urandom > msg.bin
cat sk.bin msg.bin seed_salt.bin > sk_msg.bin
./sign < sk_msg.bin > sig.bin
cat pk.bin msg.bin sig.bin > pk_msg_sig.bin
if ./verify < pk_msg_sig.bin; then
    echo "Verification succeeded"
else
    echo "Verification failed"
fi
