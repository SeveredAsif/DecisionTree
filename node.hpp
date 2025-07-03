#ifndef NODE_HPP
#define NODE_HPP
#include <vector>
#include "row.hpp"
#include <map>
#include <string>
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <sstream>

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
    unordered_map<float, int> categoryChildMap;

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
    void setCategoryChildMap(const unordered_map<float, int> &map)
    {
        categoryChildMap = map;
    }
    const unordered_map<float, int> &getCategoryChildMap() const
    {
        return categoryChildMap;
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

    void printTreeToFile(std::ofstream &out, int depth = 0, const node *parent = nullptr, bool isLeft = true) const
    {
        const char *indentStr = "    ";
        std::string indent;
        for (int i = 0; i < depth; ++i)
            indent += indentStr;
        out << indent;
        if (parent)
        {
            if (parent->getChildren().size() > 2)
            {
                // Multiway split: print value that leads to this child
                if (!rows.empty())
                {
                    float val = rows[0].row[parent->getSplitCol()];
                    out << "|-- [val = " << val << "] (col " << parent->getSplitCol() << ") ";
                }
                else
                {
                    out << "|-- [val = ?] (col " << parent->getSplitCol() << ") ";
                }
            }
            else
            {
                if (isLeft)
                    out << "|-- [L] (col " << parent->getSplitCol() << " <= " << parent->getSplitval() << ") ";
                else
                    out << "|-- [R] (col " << parent->getSplitCol() << " > " << parent->getSplitval() << ") ";
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
            out << "[Leaf] ";
            for (const auto &kv : labelCounts)
            {
                out << kv.second << " " << kv.first << "s, ";
            }
            out << "(Rows: " << rows.size() << ")\n";
        }
        else
        {
            out << "[Node]:" << "(Rows: " << rows.size() << ")\n";
            for (size_t i = 0; i < children.size(); ++i)
            {
                if (children[i])
                    children[i]->printTreeToFile(out, depth + 1, this, i == 0);
            }
        }
    }

    void exportToDot(std::ofstream &out, int &nodeId, int parentId = -1, std::string edgeLabel = "") const
    {
        int myId = nodeId++;
        std::ostringstream label;
        if (isLeaf)
        {
            std::map<int, int> labelCounts;
            for (const auto &r : rows)
                labelCounts[static_cast<int>(r.row[targetColumn])];
            label << "Leaf\n";
            for (const auto &kv : labelCounts)
                label << kv.first << ": " << kv.second << "\\n";
            label << "Rows: " << rows.size();
        }
        else
        {
            if (getChildren().size() > 2)
            {
                label << "col " << splitCol << ", multiway";
            }
            else
            {
                label << "col " << splitCol << " <= " << splitVal;
            }
        }
        out << "  node" << myId << " [label=\"" << label.str() << "\"]\n";
        if (parentId != -1)
        {
            out << "  node" << parentId << " -> node" << myId;
            if (!edgeLabel.empty())
                out << " [label=\"" << edgeLabel << "\"]";
            out << "\n";
        }
        if (!isLeaf)
        {
            for (size_t i = 0; i < children.size(); ++i)
            {
                std::string childEdgeLabel;
                if (getChildren().size() > 2)
                {
                    if (!children[i]->getRows().empty())
                    {
                        float val = children[i]->getRows()[0].row[splitCol];
                        childEdgeLabel = std::to_string(val);
                    }
                    else
                    {
                        childEdgeLabel = "?";
                    }
                }
                else
                {
                    childEdgeLabel = (i == 0) ? "<= " + std::to_string(splitVal) : "> " + std::to_string(splitVal);
                }
                if (children[i])
                    children[i]->exportToDot(out, nodeId, myId, childEdgeLabel);
            }
        }
    }
};

#endif