#ifndef ROW_HPP
#define ROW_HPP
#include <vector>
#include <iostream>
using namespace std;
class Row
{
    public:
        vector<float> row;
    Row()
    {
        //row.push_back(2105131);
    }
    void insertColumn(float a){
        row.push_back(a);
    }
    
    void print() const {
        for (size_t i = 0; i < row.size(); ++i) {
            cout << row[i];
            if (i != row.size() - 1) cout << ", ";
        }
        cout << endl;
    }
};

#endif