#include <iostream>
using namespace std;
#include <string>
#include <vector>
#include <unordered_map>
class Preprocessor
{
public:
    void preprocess(vector<vector<string>> &v)
    {
        int rowSize = v.size();
        int colSize = v[0].size();
        for (int i = 0; i < colSize; i++)
        {
            unordered_map<string, int> targetColMap;
            int uniqueTargets = 0;
            for (int j = 0; j < rowSize; j++)
            {
                if (targetColMap.find(v[j][i]) == targetColMap.end())
                {
                    // find out if the target column already converted into a number
                    targetColMap[v[j][i]] = uniqueTargets++;
                }
                v[j][i] = to_string(targetColMap[v[j][i]]);
                //cout<<" set to "<<v[j][i]<<endl;
            }
        }
    }
};