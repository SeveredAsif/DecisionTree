#include "node.hpp"
#include "math.h"
#include <algorithm>
#include <queue>
#include <unordered_set>
#include <unordered_map>

class Trainer
{
private:
    int totalColumn = 0;
    float calculateEntropy(node *&root, int unique)
    {
        // cout << "entering entropy" << endl;
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
                entropy -= total[i] * log2(total[i]);
            }
        }
        // cout << "entropy: " << entropy << endl;
        return entropy;
    }
    int findUniquevalues(node *&currNode, int splitCol)
    {
        vector<Row> rows = currNode->getRows();
        unordered_set<float> unique_set;
        for (auto row : rows)
        {
            auto r = row.row[splitCol];
            if (unique_set.find(r) == unique_set.end())
            {
                unique_set.emplace(r);
            }
        }
        return unique_set.size();
    }
    void split(node *&currNode, int splitCol, float bestSplit, vector<bool> &isCategoricalColumn)
    {
        vector<Row> currentAllRows = currNode->getRows();
        if (bestSplit != -1)
        {
            //cout << "Continuous Split on column " << splitCol << endl;
            node *leftNode = new node(false, currentAllRows[0].row.size() - 1);
            leftNode->setDepth(currNode->getDepth() + 1);
            node *rightNode = new node(false, currentAllRows[0].row.size() - 1);
            rightNode->setDepth(currNode->getDepth() + 1);
            for (int i = 0; i < currentAllRows.size(); i++)
            {
                Row row = currentAllRows[i];
                if (row.row[splitCol] <= bestSplit)
                {
                    leftNode->addRow(row);
                }
                else
                {
                    rightNode->addRow(row);
                }
            }

            if (leftNode->getRows().size() > 0)
                currNode->addChild(leftNode);
            else
                delete leftNode;
            if (rightNode->getRows().size() > 0)
                currNode->addChild(rightNode);
            else
                delete rightNode;
        }
        else
        {
            isCategoricalColumn[splitCol] = true;
            //cout << "categorical Split on column " << splitCol << endl;
            // find how many unique in split column
            unordered_map<float, int> unique_val_map;
            int unique_split_val = 0;
            for (const auto &row : currentAllRows)
            {
                float key = row.row[splitCol];
                if (unique_val_map.find(key) == unique_val_map.end())
                {
                    unique_val_map[key] = unique_split_val++;
                }
            }

            int numUniqueChildren = unique_split_val;
            vector<node *> children(numUniqueChildren);
            for (int i = 0; i < numUniqueChildren; i++)
            {
                children[i] = new node(false, currentAllRows[0].row.size() - 1);
                children[i]->setDepth(currNode->getDepth() + 1);
            }
            for (int i = 0; i < currentAllRows.size(); i++)
            {
                Row row = currentAllRows[i];

                children[unique_val_map[row.row[splitCol]]]->addRow(row);
            }
            for (int i = 0; i < children.size(); i++)
            {
                currNode->addChild(children[i]);
            }
            currNode->setCategoryChildMap(unique_val_map);
        }
    }

    pair<float, float> calculateGain(node *&currNode, int splitCol, float currentEntropy, int unique, int gainMode, int DTmode)
    {
        vector<Row> currentAllRows = currNode->getRows();
        // float returnValue = currentEntropy;
        //  at first sort the rows according to the target columns value, then take average of each consequtive sorted rows that column value
        sort(currentAllRows.begin(), currentAllRows.end(),
             [splitCol](const Row &a, const Row &b)
             {
                 return a.row[splitCol] < b.row[splitCol];
             });

        if (DTmode == 0)
        {
            float minVal = currentAllRows.front().row[splitCol];
            float maxVal = currentAllRows.back().row[splitCol];
            vector<float> splitValues;
            for (int b = 1; b < 10; ++b)
            {
                float split = minVal + b * (float)(maxVal - minVal) / 10;
                splitValues.push_back(split);
            }
            // get the rows that have less than or equal average value than the calculated average.
            // make a node using those rows.
            // calculate the new nodes' entropy
            // calculate IG -= nodes' total row/total row in currNode * calculate entropy
            //  pop those rows from current rows, repeat the process
            // for (int i = 1; i < currentAllRows.size(); ++i)
            // {
            //     float avg = (currentAllRows[i - 1].row[splitCol] + currentAllRows[i].row[splitCol]) / 2.0;
            //     splitValues.push_back(avg);
            //     node *newNode = new node(false, currentAllRows[0].row.size() - 1);
            //     for (int j = i - 1; j < currentAllRows.size(); j++)
            //     {
            //         Row row = currentAllRows[j];
            //         if (row.row[splitCol] <= avg)
            //         {
            //             newNode->addRow(row);
            //             if(j>i)
            //             {
            //                 i=j;
            //             }
            //         }
            //         else
            //         {
            //             if(j>i)
            //             {
            //                 i=j;
            //             }

            //             break;
            //         }
            //     }
            //     // calculate unique values in the new node

            //     // unordered_set<int> unique;
            //     vector<Row> newNodeRows = newNode->getRows();
            //     // cout<<"new node row count: "<<newNodeRows.size()<<endl;
            //     //  for(int i=0;i<newNodeRows.size();i++){
            //     //      Row r = newNodeRows[i];
            //     //      if(unique.find(r.row[r.row.size()-1]) ==unique.end() ){
            //     //          unique.insert(r.row[r.row.size()-1]);
            //     //      }
            //     //  }
            //     // calculate entropy of new node
            //     float newEntropy = calculateEntropy(newNode, unique);
            //     // calculate IG
            //     returnValue -= (((float)newNodeRows.size() / currentAllRows.size()) * newEntropy);
            //     // cout<<"new entropy: "<<newEntropy<<",newNodeRows.size()="<<newNodeRows.size()<<",currentAllRows.size()="<<currentAllRows.size()<<", newNodeRows.size()/currentAllRows.size()="<<(float)newNodeRows.size()/currentAllRows.size()<<", return value now: "<<returnValue<<endl;
            // }
            float currentBestGain = 0;
            float currentBestSplit = 0;
            int bestSplitLeftTotal = 0;
            int bestSplitRightTotal = 0;

            for (auto split : splitValues)
            {
                int leftTotal = 0, rightTotal = 0;
                vector<int> leftCounts(unique, 0), rightCounts(unique, 0);
                // node *leftNode = new node(false, currentAllRows[0].row.size() - 1);
                // node *rightNode = new node(false, currentAllRows[0].row.size() - 1);
                for (int i = 0; i < currentAllRows.size(); i++)
                {
                    Row row = currentAllRows[i];
                    int targetCol = row.row[row.row.size() - 1];
                    if (row.row[splitCol] <= split)
                    {
                        // leftNode->addRow(row);
                        leftCounts[targetCol]++;
                        leftTotal++;
                    }
                    else
                    {
                        rightCounts[targetCol]++;
                        rightTotal++;
                        // rightNode->addRow(row);
                    }
                }
                // cout<<"left node size: "<<leftNode->getRows().size()<<" and right node size: "<<rightNode->getRows().size()<<endl;
                // float leftEntropy = calculateEntropy(leftNode, unique);
                // float rightEntropy = calculateEntropy(rightNode, unique);
                float leftEntropy = 0, rightEntropy = 0;
                for (int i = 0; i < unique; ++i)
                {
                    if (leftCounts[i] > 0)
                        leftEntropy -= (float(leftCounts[i]) / leftTotal) * log2(float(leftCounts[i]) / leftTotal);
                    if (rightCounts[i] > 0)
                        rightEntropy -= (float(rightCounts[i]) / rightTotal) * log2(float(rightCounts[i]) / rightTotal);
                }
                // float informationGain = currentEntropy - (((float)leftNode->getRows().size() / currentAllRows.size()) * leftEntropy) - (((float)rightNode->getRows().size() / currentAllRows.size()) * rightEntropy);
                float informationGain = currentEntropy - (float(leftTotal) / currentAllRows.size()) * leftEntropy - (float(rightTotal) / currentAllRows.size()) * rightEntropy;
                // cout << "IGIG: " << informationGain << endl;
                if (informationGain > currentBestGain)
                {
                    currentBestGain = informationGain;
                    currentBestSplit = split;
                    bestSplitLeftTotal = leftTotal;
                    bestSplitRightTotal = rightTotal;
                }

                // delete leftNode;
                // delete rightNode;
            }
            // cout << "curr best gain: " << currentBestGain << " curr best split: " << currentBestSplit << endl;
            if (gainMode == 0)
            {
                return {currentBestGain, currentBestSplit};
            }
            else if (gainMode == 1)
            {
                float IV = -(((float)bestSplitLeftTotal / currentAllRows.size()) * log2(((float)bestSplitLeftTotal / currentAllRows.size())) - ((float)bestSplitRightTotal / currentAllRows.size()) * log2(((float)bestSplitRightTotal / currentAllRows.size())));
                float gainRatio = (float)currentBestGain / IV;
                return {gainRatio, currentBestSplit};
            }
            else
            {
                int totalUnique = findUniquevalues(currNode, splitCol);
                float NWIG = (currentBestGain / log2(totalUnique + 1)) * (1 - ((totalUnique - 1) / currentAllRows.size()));

                return {NWIG, currentBestSplit};
            }

            return {currentBestGain, currentBestSplit};
        }
        else
        {
            // find how many unique in split column
            unordered_map<float, int> unique_val_map;
            int unique_split_val = 0;
            for (const auto &row : currentAllRows)
            {
                float key = row.row[splitCol];
                if (unique_val_map.find(key) == unique_val_map.end())
                {
                    unique_val_map[key] = unique_split_val++;
                }
            }
            // number of unique values = number of children
            vector<vector<int>> children(unique_val_map.size(), vector<int>(unique, 0));
            vector<int> childrenRowCount(unique_val_map.size());
            // iterate all rows, count
            for (int i = 0; i < currentAllRows.size(); i++)
            {
                Row row = currentAllRows[i];
                int targetCol = row.row[row.row.size() - 1];
                int child_index = unique_val_map[row.row[splitCol]];
                childrenRowCount[child_index]++;
                children[child_index][targetCol]++;
            }
            // Now i have the necessary info to count IG
            float leftEntropy = 0, rightEntropy = 0;
            vector<float> entropy(unique_val_map.size(), 0);

            for (int i = 0; i < unique; ++i)
            {
                for (int j = 0; j < children.size(); j++)
                {
                    if (children[j][i] > 0)
                    {
                        entropy[j] -= ((float)children[j][i] / childrenRowCount[j]) * log2((float)children[j][i] / childrenRowCount[j]);
                    }
                }
            }
            float informationGain = currentEntropy;
            float IV = 0;
            for (int child = 0; child < children.size(); child++)
            {
                informationGain -= ((float)childrenRowCount[child] / currentAllRows.size()) * entropy[child];
                IV -= ((float)childrenRowCount[child] / currentAllRows.size()) * log2((float)childrenRowCount[child] / currentAllRows.size());
            }
            if (gainMode == 0)
            {
                return {informationGain, -1};
            }
            else if (gainMode == 1)
            {

                float gainRatio = (float)informationGain / IV;
                return {gainRatio, -1};
            }
            else
            {
                int totalUnique = findUniquevalues(currNode, splitCol);
                float NWIG = (informationGain / log2(totalUnique + 1)) * (1 - ((totalUnique - 1) / currentAllRows.size()));

                return {NWIG, -1};
            }
        }
    }

public:
    void train(node *&root, int unique, int maxDepth, int gainMode, vector<bool> &isCategoricalColumn)
    {
        
        if(maxDepth==0)
        {
            maxDepth=1e9;
        }
        // printf("Unique: %d\n", unique);

        // split the node using the maximum gain
        // first choose a column, calculate its gain, store it.
        int rowSize = root->getRows()[0].row.size();
        totalColumn = rowSize;
        vector<bool> doneColumn(rowSize, false);

        node *currNode = root;
        queue<node *> bfs_queue;
        bfs_queue.push(root);

        while (!bfs_queue.empty())
        {
            currNode = bfs_queue.front();
            bfs_queue.pop();

            float currentEntropy = calculateEntropy(currNode, unique);

            if (currentEntropy == 0 ||
                currNode->getIsLeaf() ||
                currNode->getDepth() >= maxDepth ||
                currNode->getRows().size() < 2)
            {
                currNode->setIsLeaf();
                continue;
            }

            float maxGain = 0;
            float bestSplit = 0;
            int bestGainAttribute = 0;

            // cout << "HERE" << endl;
            for (int i = 1; i < rowSize - 1; i++)
            {
                // cout << "no problem reaching here" << endl;
                int uniqueValues = findUniquevalues(root, i);
                int isCategorical;
                if (uniqueValues > 10)
                {
                    isCategorical = 0;
                    // cout << "unique values: " << uniqueValues << " on column" << i << ",so non categorical" << endl;
                }
                else
                {
                    isCategorical = 1;
                    // cout << "unique values: " << uniqueValues << " on column" << i << ",so categorical" << endl;
                }

                if (doneColumn[i] == true && isCategorical == 1)
                {
                    continue; // cout << "skipping column " << i << " because categorical data and this column already done" << endl;
                }

                pair<float, float> IG = calculateGain(currNode, i, currentEntropy, unique, gainMode, isCategorical);
                // cout << "currgain: " << IG.first << " , attribute: " << i << " IG.second(bestSplit): " << IG.second << endl;
                if (IG.first > maxGain)
                {
                    maxGain = IG.first;
                    // cout << "max gain: " << maxGain << endl;
                    bestSplit = IG.second;
                    // cout << "best split: " << bestSplit << endl;
                    bestGainAttribute = i;
                    // cout << "bestGainAttribute: " << bestGainAttribute << endl;
                }
            }
            // cout<<"reached"<<endl;
            // cout << "best gain attribute: " << bestGainAttribute << endl;
            // cout << "max gain: " << maxGain << endl;
            // Stop splitting if no gain or split is possible
            // if (maxGain <= 1e-6)
            // {
            //     currNode->setIsLeaf();
            //     bfs_queue.pop();
            //     continue;
            // }

            if (maxGain <= 1e-6 || bestGainAttribute == 0)
            {
                currNode->setIsLeaf();
                continue;
            }

            currNode->setSplitCol(bestGainAttribute);
            currNode->setSplitval(bestSplit);
            doneColumn[bestGainAttribute] = true;

            // split the nodes according to the best split attribute
            split(currNode, bestGainAttribute, bestSplit, isCategoricalColumn);

            for (auto child : currNode->getChildren())
            {
                if (child != nullptr)
                {
                    if (child->getDepth() < maxDepth)
                    {
                        bfs_queue.push(child);
                    }
                    else
                    {

                        child->setIsLeaf();
                    }
                }
            }
        }
    }
};