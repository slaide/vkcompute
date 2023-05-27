#include <iostream>
#include <span>
#include <cstdint>
#include <functional>

typedef size_t index_t;

template<
    index_t ROWS,
    index_t COLS,
    bool INLINE_STORAGE=(ROWS<256 && COLS<256)
>
class MatrixMemory{
    public:
        /// return nth row
        std::span<float,COLS> operator[](index_t index)const;
        std::span<float,COLS> operator[](index_t index);
};

template<
    index_t ROWS,
    index_t COLS
>
class MatrixMemory<
    ROWS,
    COLS,
    false
>{
    private:
        float* values;

    public:
        MatrixMemory(){
            values=(float*)malloc(sizeof(float)*ROWS*COLS);
        }
        MatrixMemory(const MatrixMemory&):MatrixMemory(){}
        MatrixMemory(MatrixMemory&& other){
            values=other.values;
            other.values=nullptr;
        }
        void operator=(MatrixMemory& other){
            free(values);
            values=other.values;
            other.values=nullptr;
        }
        ~MatrixMemory(){
            if(values!=nullptr)
                free(values);
        }

        /// return nth row
        std::span<float,COLS> operator[](index_t index)const{
            return std::span<float,COLS>(((float*)values)+(index*COLS),COLS);
        }
        std::span<float,COLS> operator[](index_t index){
            return std::span<float,COLS>(((float*)values)+(index*COLS),COLS);
        }
};

template<
    index_t ROWS,
    index_t COLS
>
class MatrixMemory<
    ROWS,
    COLS,
    true
>{
    private:
        float values[ROWS*COLS];

    public:
        /// return nth row
        std::span<float,COLS> operator[](index_t index)const{
            return std::span<float,COLS>((float*)values+(index*COLS),COLS);
        }
};

template<
    index_t ROWS,
    index_t COLS
>
class Matrix{
    private:
        MatrixMemory<COLS,ROWS> values;

    public:
        Matrix(){
            // Matrix::values will be default constructed, which is fine
        }
        Matrix(const Matrix& m):Matrix(){
            foreach([&](auto row,auto col){
                values[row][col]=m[row][col];
            });
        }

        /// initialise each value of the matrix so some value
        Matrix(float v):Matrix(){
            foreach([&](auto row,auto col){
                values[row][col]=v;
            });
        }

        /// first arg is row
        /// second arg is column
        typedef std::function<void(index_t,index_t)> FOREACH_FUNC_INDEX;
        /// apply some function to each element index
        void foreach(FOREACH_FUNC_INDEX func)const{
            for(index_t r=0;r<ROWS;r++){
                for(index_t c=0;c<COLS;c++){
                    func(r,c);
                }
            }
        }
        /// apply some function to each element index
        void foreach(FOREACH_FUNC_INDEX func){
            for(index_t r=0;r<ROWS;r++){
                for(index_t c=0;c<COLS;c++){
                    func(r,c);
                }
            }
        }

        /// apply some function to each element
        void foreach(std::function<void(float&)> func){
            for(index_t r=0;r<ROWS;r++){
                for(index_t c=0;c<COLS;c++){
                    func(values[r][c]);
                }
            }
        }
        /// apply some function to each element
        void foreach(std::function<void(const float&)> func)const{
            for(index_t r=0;r<ROWS;r++){
                for(index_t c=0;c<COLS;c++){
                    func(values[r][c]);
                }
            }
        }

        // inherit indexing operator from underlying memory wrappers
        auto operator[](auto index)const{
            return values[index];
        }
        auto operator[](auto index){
            return values[index];
        }

        // sum value type is double to avoid early float precsion issues
        double sum()const{
            double s=0.0;
            foreach([&](auto r,auto c){
                s+=values[r][c];
            });
            return s;
        }

        Matrix<ROWS,COLS> operator*(float v)const{
            auto ret=Matrix<ROWS,COLS>();
            for(index_t r=0;r<ROWS;r++){
                for(index_t c=0;c<COLS;c++){
                    ret[r][c]=values[r][c]*v;
                }
            }
            return ret;
        }
};

int main(int, char**) {
    {
        constexpr index_t MSIZE=1024*16;
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
}
