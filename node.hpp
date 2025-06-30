#include <vector>
#include "row.hpp"
class node
{
private:
    vector<Row> rows;
    bool isLeaf;
    int targetColumn;
    vector<node *> children;

public:
    node(bool isLeaf, int targetColumn)
        : isLeaf(isLeaf), targetColumn(targetColumn) {}

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

    int getTargetColumn()
    {
        return targetColumn;
    }
    void setTargetColumn(int i){
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
};