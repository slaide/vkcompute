#pragma once

#include <span>
#include <cstdint>
#include <string>
#include <functional>
#include <iostream>

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

        /// return nth column
        float* operator[](index_t index)const{
            return ((float*)(&values[index*ROWS]));
        }
        float* operator[](index_t index){
            return ((float*)(&values[index*ROWS]));
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
    public:
        float values[ROWS*COLS];

    public:
        /// return nth column
        float* operator[](index_t index)const{
            return (float*)(&(values[index*ROWS]));
        }
};


/// matrix data layout is column-major
template<
    index_t ROWS,
    index_t COLS
>
class Matrix{
    public:
        MatrixMemory<COLS,ROWS> values;

    public:
        Matrix(){
            // Matrix::values will be default constructed, which is fine
        }
        Matrix(const float (&data)[ROWS*COLS]){
            foreach([&](auto c,auto r){
                values[c][r]=data[c*ROWS+r];
            });
        }
        Matrix(const Matrix& m):Matrix(){
            foreach([&](auto col,auto row){
                values[col][row]=m[col][row];
            });
        }

        /// initialise each value of the matrix so some value
        Matrix(float v):Matrix(){
            foreach([&](auto col,auto row){
                values[col][row]=v;
            });
        }

        /// first arg is row
        /// second arg is column
        typedef std::function<void(index_t,index_t)> FOREACH_FUNC_INDEX;
        /// apply some function to each element index
        void foreach(FOREACH_FUNC_INDEX func)const{
            for(index_t c=0;c<COLS;c++){
                for(index_t r=0;r<ROWS;r++){
                    func(c,r);
                }
            }
        }
        /// apply some function to each element index
        void foreach(FOREACH_FUNC_INDEX func){
            for(index_t c=0;c<COLS;c++){
                for(index_t r=0;r<ROWS;r++){
                    func(c,r);
                }
            }
        }

        /// apply some function to each element
        void foreach(std::function<void(float&)> func){
            foreach([&](auto c,auto r){
                func(values[c][r]);
            });
        }
        /// apply some function to each element
        void foreach(std::function<void(const float&)> func)const{
            foreach([&](auto c,auto r){
                func(values[c][r]);
            });
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
            auto ret = Matrix<ROWS, COLS>();
            foreach([&](auto r,auto c){
                ret[r][c] = values[r][c] * v;
            });
            return ret;
        }

        template<index_t R_COLS>
        Matrix<R_COLS,ROWS> operator*(const Matrix<COLS,R_COLS> &right)const{
            auto ret = Matrix<R_COLS,ROWS>{};
            for(index_t r=0;r<R_COLS;r++){
                for(index_t c=0;c<ROWS;c++){
                    float e=0.0;
                    for(index_t i=0;i<COLS;i++){
                        e+=values[c][i]*right[i][r];
                    }
                    ret[r][c]=e;
                }
            }
            return ret;
        }

        Matrix<COLS,ROWS> transposed()const{
            Matrix<COLS,ROWS> ret;
            foreach([&](auto r,auto c){
                ret[c][r] = values[r][c];
            });
            return ret;
        }

        std::string string()const{
            std::string rows[ROWS];
            foreach([&](auto c,auto r){
                if(c>0){
                    rows[r]+="  ";
                }
                rows[r]+=std::to_string(values[c][r]);
            });
            std::string ret=std::to_string(ROWS)+" rows by "+std::to_string(COLS)+" columns:\n";
            for(auto i=0;i<ROWS*COLS;i++){
                std::cout<<i<<" : "<<values.values[i]<<std::endl;
            }
            for(auto row:rows){
                ret+=row+"\n";
            }
            return ret;
        }
};

class Vec3:public Matrix<3,1>{};
class Vec4:public Matrix<4,1>{};
class Mat4:public Matrix<4,4>{};
