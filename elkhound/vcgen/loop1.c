int main() {
    int x,y;
    int arr[10];
    int *ptr;

    for (x=0;x<5;x++) {
        arr[x] = x;
    } 
    for (x=4;x>=0;x--) {
        assert(arr[x] == x);
    } 

    return 0; 
}
