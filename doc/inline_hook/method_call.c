#include <stdio.h>
 
int add(int a,int b){
 register int c = a + b;
 return c;
}
 
int main(){
  int a = 100;
  int b = 101;
  int c = add(a,b);
  return c;
} 
