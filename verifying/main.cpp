#include <iostream>
#include <manif/impl/rn/Rn.h>
#include <manif/impl/se3/SE3.h>
#include <manif/impl/so3/SO3.h>
#include <manif/impl/so3/SO3Tangent.h>

int main(){

    manif::SO3d a{0,1,0, 0};

    manif::SO3Tangentd SO3tan = a.log(); 

    std::cout << SO3tan << std::endl;

    return 0;
}