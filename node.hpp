#ifndef NODE_HPP
#define NODE_HPP
#include <vector>
#include "row.hpp"
#include <map>
#include <string>
#include <iostream>

class node
{
private:
    vector<Row> rows;
    bool isLeaf;
    int targetColumn;
    vector<node *> children;
    int splitCol;
    int splitVal;

public:
    node(bool isLeaf, int targetColumn)
        : isLeaf(isLeaf), targetColumn(targetColumn), splitCol(0), splitVal(0) {}

    void addRow(Row &row)
    {
        rows.push_back(row);
    }

    vector<Row> &getRows()
    {
        return rows;
    }

    bool getIsLeaf()
    {
        return isLeaf;
    }
    void setIsLeaf()
    {
        isLeaf = true;
    }

    int getTargetColumn()
    {
        return targetColumn;
    }
    void setTargetColumn(int i)
    {
        this->targetColumn = i;
    }
    void addChild(node *child)
    {
        children.push_back(child);
    }
    vector<node *> &getChildren()
    {
        return children;
    }
    void print() const
    {
        for (const auto &r : rows)
        {
            r.print();
        }
    }
    void setSplitCol(int a)
    {
        this->splitCol = a;
    }
    void setSplitval(int a)
    {
        this->splitVal = a;
    }
    int getSplitCol()
    {
        return this->splitCol;
    }
    int getSplitval()
    {
        return this->splitVal;
    }

    void printTree(int depth = 0, std::string branch = "", std::string splitDesc = "") const
    {
        // Draw the branch lines
        if (depth > 0)
        {
            std::cout << branch.substr(0, branch.size() - 3);
            std::cout << (branch.back() == '|' ? "|--" : "`--");
            std::cout << splitDesc;
        }
        if (isLeaf)
        {
            // Count label frequencies
            std::map<int, int> labelCounts;
            for (const auto &r : rows)
            {
                int label = static_cast<int>(r.row[targetColumn]);
                labelCounts[label]++;
            }
            std::cout << "[Leaf] ";
            for (const auto &kv : labelCounts)
            {
                std::cout << kv.second << " " << kv.first << "s, ";
            }
            std::cout << "(Rows: " << rows.size() << ")\n";
        }
        else
        {
            std::cout << "[Node] Split: (col " << splitCol << ", <= " << splitVal << ") (Rows: " << rows.size() << ")\n";
            for (size_t i = 0; i < children.size(); ++i)
            {
                std::string nextBranch = branch;
                if (depth > 0)
                    nextBranch += (branch.back() == '|' ? "|   " : "    ");
                nextBranch += (i == children.size() - 1 ? "`" : "|");
                std::string childSplitDesc;
                if (i == 0)
                    childSplitDesc = " (col " + std::to_string(splitCol) + " <= " + std::to_string(splitVal) + ") ";
                else
                    childSplitDesc = " (col " + std::to_string(splitCol) + " > " + std::to_string(splitVal) + ") ";
                if (children[i])
                    children[i]->printTree(depth + 1, nextBranch, childSplitDesc);
            }
        }
    }
};

#endif