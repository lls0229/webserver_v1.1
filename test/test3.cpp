#include<iostream>
#include<string>
using namespace std;
class Person{
         private:
                 string m_name;
                 int m_age;
         public:
                 Person(string name, int age):
                         m_name(name),m_age(age){}
                 void getName(){
                         cout << "my name is " << this->m_name << endl;
                 }
                 void getAge(){
                         cout << "my age is " << this->m_age << endl;
                 }
};
int main(){
         Person p1("lls",23);
         Person p2("chp",21);
         p1.getName();
         p1.getAge();
         p2.getName();
         p2.getAge();
         return 0;
} 
