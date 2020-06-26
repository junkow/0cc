// #include <stdio.h>
int t[8] = {-1};
int sol = 0;

int absval(int a) {
    if(a < 0) {
        // printf("actual: %d, expect: %d \n", ~(a-1), -a);
        return ~(a-1);
    }
    return a;
}

int empty(int i) {
    int j = 0;
    while((t[i] != t[j]) && (absval(t[i] - t[j]) != (i-j)) && j<8) j++;
    if(i==j)
        return 1;
    else
        return 0;
}

int queens(int i) {
    for(t[i] = 0; t[i] < 8; t[i]++) {
        if(empty(i)) {
            if(i == 7) {
                ++sol;
            } else {
                queens(i+1);
            }
        }
    }
}

int queenssol8() {
// int main() {
    queens(0);
    // printf("%d \n", sol);
    return sol;
}