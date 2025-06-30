#include "node.hpp"
#include "math.h"
#include <algorithm>
#include <queue>

class Trainer
{
private:
    float calculateEntropy(node *&root, int unique)
    {
        int S = root->getRows().size();
        float *total = (float *)malloc(unique);
        for (int i = 0; i < unique; i++)
        {
            total[i] = 0;
        }

        int rowSize = root->getRows()[0].row.size();
        int rootSize = root->getRows().size();
        auto rows = root->getRows();
        int total_nodes = 0;
        float entropy = 0;
        for (int i = 0; i < rootSize; i++)
        {
            int target = rows[i].row[rowSize - 1];
            int id = rows[i].row[0];
            // printf("id:%d, target class: %d for i+1=%d\n",id,target,i+1);
            total[target]++;
            total_nodes++;
            // printf("now target %d is total: %d\n",target,total[target]);
        }
        for (int i = 0; i < unique; i++)
        {
            // cout<<"total: "<<total[i]<<endl;
            total[i] = total[i] / total_nodes;
            // cout<<total[i]*log(total[i])<<endl;
            if (total[i] != 0)
            {
                entropy -= total[i] * log(total[i]);
            }
        }
        // cout<<"entropy: "<<entropy<<endl;
        return entropy;
    }
    void split(node *&currNode, int splitCol)
    {
        vector<Row> currentAllRows = currNode->getRows();

        // at first sort the rows according to the target columns value, then take average of each consequtive sorted rows that column value
        sort(currentAllRows.begin(), currentAllRows.end(),
             [splitCol](const Row &a, const Row &b)
             {
                 return a.row[splitCol] < b.row[splitCol];
             });
        for (int i = 1; i < currentAllRows.size(); ++i)
        {
            float avg = (currentAllRows[i - 1].row[splitCol] + currentAllRows[i].row[splitCol]) / 2.0;
            node *newNode = new node(false, currentAllRows[0].row.size() - 1);
            //newNode->setSplitCol(splitCol);
            newNode->setSplitval(avg);
            for (int j = i - 1; j < currentAllRows.size(); j++)
            {
                Row row = currentAllRows[j];
                if (row.row[splitCol] <= avg)
                {
                    newNode->addRow(row);
                }
                else
                {
                    i = j;
                    break;
                }
            }
            currNode->addChild(newNode);
        }
    }

    float calculateGain(node *&currNode, int splitCol, float currentEntropy, int unique)
    {
        vector<Row> currentAllRows = currNode->getRows();
        float returnValue = currentEntropy;
        // at first sort the rows according to the target columns value, then take average of each consequtive sorted rows that column value
        sort(currentAllRows.begin(), currentAllRows.end(),
             [splitCol](const Row &a, const Row &b)
             {
                 return a.row[splitCol] < b.row[splitCol];
             });
        // get the rows that have less than or equal average value than the calculated average.
        // make a node using those rows.
        // calculate the new nodes' entropy
        // calculate IG -= nodes' total row/total row in currNode * calculate entropy
        //  pop those rows from current rows, repeat the process
        for (int i = 1; i < currentAllRows.size(); ++i)
        {
            float avg = (currentAllRows[i - 1].row[splitCol] + currentAllRows[i].row[splitCol]) / 2.0;
            node *newNode = new node(false, currentAllRows[0].row.size() - 1);
            for (int j = i - 1; j < currentAllRows.size(); j++)
            {
                Row row = currentAllRows[j];
                if (row.row[splitCol] <= avg)
                {
                    newNode->addRow(row);
                }
                else
                {
                    i = j;
                    break;
                }
            }
            // calculate unique values in the new node

            // unordered_set<int> unique;
            vector<Row> newNodeRows = newNode->getRows();
            // cout<<"new node row count: "<<newNodeRows.size()<<endl;
            //  for(int i=0;i<newNodeRows.size();i++){
            //      Row r = newNodeRows[i];
            //      if(unique.find(r.row[r.row.size()-1]) ==unique.end() ){
            //          unique.insert(r.row[r.row.size()-1]);
            //      }
            //  }
            // calculate entropy of new node
            float newEntropy = calculateEntropy(newNode, unique);
            // calculate IG
            returnValue -= (((float)newNodeRows.size() / currentAllRows.size()) * newEntropy);
            // cout<<"new entropy: "<<newEntropy<<",newNodeRows.size()="<<newNodeRows.size()<<",currentAllRows.size()="<<currentAllRows.size()<<", newNodeRows.size()/currentAllRows.size()="<<(float)newNodeRows.size()/currentAllRows.size()<<", return value now: "<<returnValue<<endl;
        }
        return returnValue;
    }

public:
    void train(node *&root, int unique)
    {
        printf("Unique: %d\n", unique);

        // split the node using the maximum gain
        // first choose a column, calculate its gain, store it.
        int rowSize = root->getRows()[0].row.size();

        node *currNode = root;
        queue<node *> bfs_queue;
        bfs_queue.push(root);
        while (!bfs_queue.empty())
        {
            currNode = bfs_queue.front();
            float currentEntropy = calculateEntropy(currNode, unique);
            if(currentEntropy==0 || currNode->getIsLeaf())
            {
                currNode->setIsLeaf();
                bfs_queue.pop();
                continue;
            }
            float maxGain = 0;
            int bestGainAttribute = 0;
            cout << "HERE" << endl;
            for (int i = 1; i < rowSize - 1; i++)
            {
                float currGain = calculateGain(currNode, i, currentEntropy, unique);
                cout << "currgain: " << currGain << " ,attribute: " << i << endl;
                if (currGain > maxGain)
                {
                    maxGain = currGain;
                    bestGainAttribute = i;
                }
            }
            // cout<<"reached"<<endl;
            cout << "best gain attribute: " << bestGainAttribute << endl;
            cout << "max gain: " << maxGain << endl;
            currNode->setSplitCol(bestGainAttribute);

            // split the nodes according to the best split attribute
            split(currNode, bestGainAttribute);
            for (auto child : currNode->getChildren())
            {
                bfs_queue.push(child);
            }
            bfs_queue.pop();
        }
    }
};