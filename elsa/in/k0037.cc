// new array of pointers

// originally found in package achilles

// Parse error (state 253) at [

// ERR-MATCH: Parse error.*at \[$

int main() {
    int ** pointer_array = new (int*)[42];
}
