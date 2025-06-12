
gcc -o quicksort main.c -lpthread


for size in 10000000 20000000 30000000 40000000 50000000
do
    echo "******** Problem Size: $size ********"

    
    for threads in 1 2 4 8 16 32
    do
        echo "====================================="
        echo "Running with $threads threads"
        echo "====================================="
        ./quicksort $threads $size
        echo ""
    done

    echo "====================================="
    echo ""
done