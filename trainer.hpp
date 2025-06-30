#include "node.hpp"
class Trainer
{
private:
    float calculateEntropy(node *&root, int unique)
    {
        int S = root->getRows().size();
        int *total = (int *)malloc(unique);
        for (int i = 0; i < unique; i++)
        {
            total[i] = 0;
        }

        int rowSize = root->getRows()[0].row.size();
        int rootSize = root->getRows().size();
        auto rows = root->getRows();
        for (int i = 0; i < rootSize; i++)
        {
            int target = rows[i].row[rowSize - 1] - 1;
            total[target]++;
        }
        for (int i = 0; i < unique; i++)
        {
            cout<<total[i]<<endl;
        }
    }

public:
    void train(node *&root, int unique)
    {
        calculateEntropy(root,unique);
    }
};