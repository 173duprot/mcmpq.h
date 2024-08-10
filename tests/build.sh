
echo Building basic.c
gcc -Wall -Wextra -Wpedantic -Wshadow -Wcast-align -Wstrict-overflow=5 \
    -Wswitch-default -Wswitch-enum -Wunreachable-code -Wconversion \
    -Wfloat-equal -Wpointer-arith -Wstrict-prototypes -Wformat=2 \
    -Wshift-overflow -Wformat=2 -Wnull-dereference -Wduplicated-cond \
    -Wduplicated-branches -Wlogical-op -Wuseless-cast -Wunsafe-loop-optimizations \
    -fsanitize=thread -fsanitize=undefined `#-fsanitize=address` -fstack-protector-strong \
    -fno-common -O0 -o basic basic.c -lpthread

echo Building fuzz.c
gcc -Wall -Wextra -Wpedantic -Wshadow -Wcast-align -Wstrict-overflow=5 \
    -Wswitch-default -Wswitch-enum -Wunreachable-code -Wconversion \
    -Wfloat-equal -Wpointer-arith -Wstrict-prototypes -Wformat=2 \
    -Wshift-overflow -Wformat-signedness -Wnull-dereference -Wduplicated-cond \
    -Wduplicated-branches -Wlogical-op -Wuseless-cast -Wunsafe-loop-optimizations \
    -fsanitize=thread -fsanitize=undefined `#-fsanitize=address` -fstack-protector-strong \
    -fno-common -O0 -o fuzz fuzz.c -lpthread
