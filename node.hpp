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
    float splitVal;
    int depth;

public:
    node(bool isLeaf, int targetColumn)
        : isLeaf(isLeaf), targetColumn(targetColumn), splitCol(0), splitVal(0) { this->depth = 0; }

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
    int getDepth()
    {
        return this->depth;
    }
    void setDepth(int d)
    {
        this->depth = d;
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
    const vector<node *> &getChildren() const
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
    void setSplitval(float a)
    {
        this->splitVal = a;
    }
    int getSplitCol() const
    {
        return this->splitCol;
    }
    float getSplitval() const
    {
        return this->splitVal;
    }

    void printTree(int depth = 0, const node *parent = nullptr, bool isLeft = true) const
    {
        // ANSI color codes for levels
        const char *colors[] = {"\033[0m", "\033[36m", "\033[32m", "\033[33m", "\033[35m", "\033[31m"};
        int colorIdx = depth % 6;
        std::string indent;
        for (int i = 0; i < depth; ++i)
            indent += "    ";
        std::cout << colors[colorIdx] << indent;
        if (parent)
        {
            if (parent->getChildren().size() > 2)
            {
                // Multiway split: print value that leads to this child
                if (!rows.empty())
                {
                    float val = rows[0].row[parent->getSplitCol()];
                    std::cout << "|-- [val = " << val << "] (col " << parent->getSplitCol() << ") ";
                }
                else
                {
                    std::cout << "|-- [val = ?] (col " << parent->getSplitCol() << ") ";
                }
            }
            else
            {
                if (isLeft)
                    std::cout << "|-- [L] (col " << parent->getSplitCol() << " <= " << parent->getSplitval() << ") ";
                else
                    std::cout << "|-- [R] (col " << parent->getSplitCol() << " > " << parent->getSplitval() << ") ";
            }
        }
        if (isLeaf)
        {
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
            std::cout << "(Rows: " << rows.size() << ")\033[0m\n";
        }
        else
        {
            std::cout << "[Node]:" << "(Rows: " << rows.size() << ")\033[0m\n";
            for (size_t i = 0; i < children.size(); ++i)
            {
                if (children[i])
                    children[i]->printTree(depth + 1, this, i == 0);
            }
        }
    }
};

#endif