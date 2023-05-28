#include <iostream>
#include <matrix.hpp>
#include <application.h>

int main(int, char**) {
    {
        constexpr index_t MSIZE=1024*2;
        Matrix<MSIZE,MSIZE> somematrix(2);

        std::cout
            << "runtime sum " << somematrix.sum() << "\n"
            << "expected    " << MSIZE*MSIZE*2.0
            << std::endl;
    }
    {
        constexpr index_t MSIZE=1024;
        Matrix<MSIZE,MSIZE> somematrix(2);
        auto newmatrix=somematrix*2.0;

        std::cout
            << "runtime sum " << newmatrix.sum() << "\n"
            << "expected    " << MSIZE*MSIZE*4.0
            << std::endl;
    }
    try{
        auto app=std::make_shared<Application>();

        Vec3 v{1.0f};
        std::cout<<v.string()<<std::endl;
        std::cout<<v.transposed().string()<<std::endl;

        auto m1=Matrix<2,3>{{
            1,4,
            2,5,
            3,6
        }};
        auto m2=Matrix<3,2>{{
            7,10,
            8,11,
            9,12
        }};
        std::cout<<m1.string()<<std::endl;
        std::cout<<m2.string()<<std::endl;
        std::cout<<(m1*m2).string()<<std::endl;

        {
            Matrix<3,1> m{1.0};
            for(int i=0;i<3;i++){
                std::cout<<std::to_string(i)<<" = "<<std::to_string(m.values.values[i])<<std::endl;
            }
        }
        {
            Matrix<1,3> m{1.0};
            for(int i=0;i<3;i++){
                std::cout<<std::to_string(i)<<" = "<<std::to_string(m.values.values[i])<<std::endl;
            }
        }
        {
            Matrix<4,2> m{1.0};
            for(int i=0;i<8;i++){
                std::cout<<std::to_string(i)<<" = "<<std::to_string(m.values.values[i])<<" - ";
            }
            std::cout<<std::endl;
        }
        {
            Matrix<2,4> m{1.0};
            for(int i=0;i<8;i++){
                std::cout<<std::to_string(i)<<" = "<<std::to_string(m.values.values[i])<<" - ";
            }
            std::cout<<std::endl;
        }

        app->run();
    }catch(VulkanError *vk_error){
        std::cout<<vk_error->info()<<std::endl;
    };
}
