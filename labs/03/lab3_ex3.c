int source[] = {3, 1, 4, 1, 5, 9, 0};
int dest[10];

int main ( ) {
    int k;
    for (k=0; source[k]!=0; k++) {
    dest[k] = source[k];
    }
    return 0;
}

// before loop
// load to $3 the first element of source
// if $3 is 0 go to L2

// L3
// incement source by 4
// store $3 in destination
// increment destination by 4
// load to $3 another element of source
// if $3 is not 0 go back to L3
