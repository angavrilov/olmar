// calling a templated function pointer

// error: you can't use an expression of type `funcType &' as a function

// originally found in package aprsd

template<typename funcType>
void generate(funcType func) {
    func();
}
