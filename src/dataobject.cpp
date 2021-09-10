//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, Lawrence Livermore National Security, LLC. Produced
// at the Lawrence Livermore National Laboratory. Written by Alfredo
// Gimenez (alfredo.gimenez@gmail.com). LLNL-CODE-663358. All rights
// reserved.
//
// This file is part of MemAxes. For details, see
// https://github.com/scalability-tools/MemAxes
//
// Please also read this link – Our Notice and GNU Lesser General Public
// License. This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License (as
// published by the Free Software Foundation) version 2.1 dated February
// 1999.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
// conditions of the GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// OUR NOTICE AND TERMS AND CONDITIONS OF THE GNU GENERAL PUBLIC LICENSE
// Our Preamble Notice
// A. This notice is required to be provided under our contract with the
// U.S. Department of Energy (DOE). This work was produced at the Lawrence
// Livermore National Laboratory under Contract No. DE-AC52-07NA27344 with
// the DOE.
// B. Neither the United States Government nor Lawrence Livermore National
// Security, LLC nor any of their employees, makes any warranty, express or
// implied, or assumes any liability or responsibility for the accuracy,
// completeness, or usefulness of any information, apparatus, product, or
// process disclosed, or represents that its use would not infringe
// privately-owned rights.
//////////////////////////////////////////////////////////////////////////////

#include "dataobject.h"
#include "parseUtil.h"

#include <iostream>
#include <algorithm>
#include <functional>

#include "hwloc.hpp"

#include <QFile>
#include <QTextStream>

DataObject::DataObject()
{
    numDimensions = 0;
    numElements = 0;
    numSelected = 0;
    numVisible = 0;

    topo = NULL;

    selMode = MODE_NEW;
    selGroup = 1;
}

int DataObject::loadHardwareTopology(QString filename)
{
    qDebug( "loadHardwareTopology" );

    topo = new Topology();
    int err = addParsedHwlocTopo(topo, filename.toUtf8().constData(), 1); //adds topo to a next node
    //topo->PrintSubtree(0);

    //TODO temporary CPU
    cpu = (Chip*)(topo->GetChild(0)->GetChild(1));


    //create empty metadata items "sampleSets" and "transactions" for each Compoenent
    vector<Component*> allComponents;
    cpu->GetSubtreeNodeList(&allComponents);
    qDebug( "loadHardwareTopology = %lu", allComponents.size());
    for(int i=0; i<allComponents.size(); i++)
    {
        allComponents[i]->metadata["sampleSets"] = (void*) new QMap<DataObject*,SampleSet>();
        allComponents[i]->metadata["transactions"] = (void*) new int();
        qDebug( "collectTopoSamplesxx = %p", allComponents[i]->metadata["sampleSets"]);
    }
    return err;
    // topo = new hwTopo();
    // int err = topo->loadHardwareTopologyFromXML(filename);
    // return err;
}

int DataObject::loadData(QString filename)
{
    qDebug( "C Style Debug Message" );
    int err = parseCSVFile(filename);
    qDebug( "loadData - error %d", err);
    if(err)
        return err;

    calcStatistics();
    qDebug( "loadData2");
    constructSortedLists();
    qDebug( "loadData3");

    return 0;
}

void DataObject::allocate()
{
    numDimensions = meta.size();
    numElements = vals.size() / numDimensions;

    begin = vals.begin();
    end = vals.end();

    visibility.resize(numElements);
    visibility.fill(VISIBLE);

    selectionGroup.resize(numElements);
    selectionGroup.fill(0); // all belong to 0 (unselected)

    selectionSets.push_back(ElemSet());
    selectionSets.push_back(ElemSet());
}

int DataObject::selected(ElemIndex index)
{
    return selectionGroup.at((int)index);
}

bool DataObject::visible(ElemIndex index)
{
    return visibility.at((int)index);
}

bool DataObject::selectionDefined()
{
    return numSelected > 0;
}

void DataObject::selectData(ElemIndex index, int group)
{
    if(visible(index))
    {
        selectionGroup[index] = group;
        selectionSets.at(group).insert(index);
        numSelected++;
    }
}

void DataObject::selectAll(int group)
{
    selectionGroup.fill(group);

    for(ElemIndex i=0; i<numElements; i++)
    {
        selectionSets.at(group).insert(i);
    }

    numSelected = numElements;
}

void DataObject::deselectAll()
{
    selectionGroup.fill(0);

    for(unsigned int i=0; i<selectionSets.size(); i++)
        selectionSets.at(i).clear();

    numSelected = 0;
}

void DataObject::selectAllVisible(int group)
{
    ElemIndex elem;
    for(elem=0; elem<numElements; elem++)
    {
        if(visible(elem))
            selectData(elem,group);
    }
}

void DataObject::showData(unsigned int index)
{
    if(!visible(index))
    {
        visibility[index] = VISIBLE;
        numVisible++;
    }
}

void DataObject::hideData(unsigned int index)
{
    if(visible(index))
    {
        visibility[index] = INVISIBLE;
        numVisible--;
    }
}

void DataObject::showAll()
{
    visibility.fill(VISIBLE);
    numVisible = numElements;
}

void DataObject::hideAll()
{
    visibility.fill(INVISIBLE);
    numVisible = 0;
}

void DataObject::selectBySourceFileName(QString str, int group)
{
    ElemSet selSet;
    ElemIndex elem;
    QVector<qreal>::Iterator p;
    for(elem=0, p=this->begin; p!=this->end; elem++, p+=this->numDimensions)
    {
        if(fileNames[elem] == str)
            selSet.insert(elem);
    }
    selectSet(selSet, group);
}

struct indexedValueLtFunctor
{
    indexedValueLtFunctor(const struct indexedValue _it) : it(_it) {}

    struct indexedValue it;

    bool operator()(const struct indexedValue &other)
    {
        return it.val < other.val;
    }
};

void DataObject::selectByDimRange(int dim, qreal vmin, qreal vmax, int group)
{
    ElemSet selSet;
    std::vector<indexedValue>::iterator itMin;
    std::vector<indexedValue>::iterator itMax;

    struct indexedValue ivMinQuery;
    ivMinQuery.val = vmin;

    struct indexedValue ivMaxQuery;
    ivMaxQuery.val = vmax;

    if(ivMinQuery.val <= this->minimumValues[dim])
    {
        itMin = dimSortedLists.at(dim).begin();
    }
    else
    {
        itMin = std::find_if(dimSortedLists.at(dim).begin(),
                             dimSortedLists.at(dim).end(),
                             indexedValueLtFunctor(ivMinQuery));
    }

    if(ivMaxQuery.val >= this->maximumValues[dim])
    {
        itMax = dimSortedLists.at(dim).end();
    }
    else
    {
        itMax = std::find_if(dimSortedLists.at(dim).begin(),
                             dimSortedLists.at(dim).end(),
                             indexedValueLtFunctor(ivMaxQuery));
    }

    for(/*itMin*/; itMin != itMax; itMin++)
    {
        selSet.insert(itMin->idx);
    }

    selectSet(selSet,group);
}

void DataObject::selectByMultiDimRange(QVector<int> dims, QVector<qreal> mins, QVector<qreal> maxes, int group)
{
    ElemSet selSet;
    for(int d=0; d<dims.size(); d++)
    {
        int dim = dims[d];
        std::vector<indexedValue>::iterator itMin;
        std::vector<indexedValue>::iterator itMax;

        struct indexedValue ivMinQuery;
        ivMinQuery.val = mins[d];

        struct indexedValue ivMaxQuery;
        ivMaxQuery.val = maxes[d];

        if(ivMinQuery.val <= this->minimumValues[dim])
        {
            itMin = dimSortedLists.at(dim).begin();
        }
        else
        {
            itMin = std::find_if(dimSortedLists.at(dim).begin(),
                                 dimSortedLists.at(dim).end(),
                                 indexedValueLtFunctor(ivMinQuery));
        }

        if(ivMaxQuery.val >= this->maximumValues[dim])
        {
            itMax = dimSortedLists.at(dim).end();
        }
        else
        {
            itMax = std::find_if(dimSortedLists.at(dim).begin(),
                                 dimSortedLists.at(dim).end(),
                                 indexedValueLtFunctor(ivMaxQuery));
        }

        for(/*itMin*/; itMin != itMax; itMin++)
        {
            selSet.insert(itMin->idx);
        }
    }

    selectSet(selSet,group);
}

void DataObject::selectByVarName(QString str, int group)
{
    ElemSet selSet;

    ElemIndex elem;
    QVector<qreal>::Iterator p;
    for(elem=0, p=this->begin; p!=this->end; elem++, p+=this->numDimensions)
    {
        if(varNames[elem] == str)
            selSet.insert(elem);
    }

    selectSet(selSet,group);
}

void DataObject::selectByResource(Component *c, int group)
{
    selectSet( (*((QMap<DataObject*,SampleSet>*)c->metadata["sampleSets"]))[this].totSamples, group);
    //selectSet(node->sampleSets[this].totSamples,group);
}

void DataObject::hideSelected()
{
    ElemIndex elem;
    for(elem=0; elem<numElements; elem++)
    {
        if(selected(elem))
            hideData(elem);
    }
}

void DataObject::hideUnselected()
{
    ElemIndex elem;
    for(elem=0; elem<numElements; elem++)
    {
        if(!selected(elem) && visible(elem))
            hideData(elem);
    }
}

void DataObject::selectSet(ElemSet &s, int group)
{
    ElemSet *newSel = NULL;
    if(selMode == MODE_NEW)
    {
        newSel = &s;
    }
    else if(selMode == MODE_APPEND)
    {
        newSel = new ElemSet();
        std::set_union(selectionSets.at(group).begin(),
                       selectionSets.at(group).end(),
                       s.begin(),
                       s.end(),
                       std::inserter(*newSel,
                                     newSel->begin()));
    }
    else if(selMode == MODE_FILTER)
    {
        newSel = new ElemSet();
        std::set_intersection(selectionSets.at(group).begin(),
                              selectionSets.at(group).end(),
                              s.begin(),
                              s.end(),
                              std::inserter(*newSel,
                                            newSel->begin()));
    }
    else
    {
        std::cerr << "MANATEES! EVERYWHERE!" << std::endl;
        return;
    }

    // Reprocess groups
    selectionSets.at(group).clear();
    selectionGroup.fill(0);

    for(ElemSet::iterator it = newSel->begin();
        it != newSel->end();
        it++)
    {
        selectData(*it,group);
    }
}

void DataObject::collectTopoSamples()
{
    // Reset info
    vector<Component*> allComponents;
    cpu->GetSubtreeNodeList(&allComponents);
    qDebug( "cpu [%d] = %p", cpu->GetComponentType(), cpu);

    qDebug( "collectTopoSamples");
    map<int,Thread*> threadIdMap;
    for(int i=0; i<allComponents.size(); i++)
    {
        if(allComponents[i]->GetComponentType() == SYS_TOPO_COMPONENT_THREAD)
        {
            threadIdMap[allComponents[i]->GetId()] = (Thread*)allComponents[i];
            qDebug( "threadIdMap [%d] = %p", allComponents[i]->GetId(), (Thread*)allComponents[i]);
        }
    }
    qDebug( "collectTopoSamples1 = %lu", allComponents.size());


    for(int i=0; i<allComponents.size(); i++)
    {
        qDebug( "collectTopoSamplesxx = %p", allComponents[i]->metadata["sampleSets"]);
        QMap<DataObject*,SampleSet>* sampleSets = (QMap<DataObject*,SampleSet>*)allComponents[i]->metadata["sampleSets"];
        (*sampleSets)[this].totCycles = 0;
        (*sampleSets)[this].selCycles = 0;
        (*sampleSets)[this].totSamples.clear();
        (*sampleSets)[this].selSamples.clear();
        *(int*)(allComponents[i]->metadata["transactions"]) = 0;
        // topo->allHardwareResourceNodes[i]->sampleSets[this].totCycles = 0;
        // topo->allHardwareResourceNodes[i]->sampleSets[this].selCycles = 0;
        // topo->allHardwareResourceNodes[i]->sampleSets[this].totSamples.clear();
        // topo->allHardwareResourceNodes[i]->sampleSets[this].selSamples.clear();
        // topo->allHardwareResourceNodes[i]->transactions = 0;
    }
    qDebug( "collectTopoSamples2");

    // Go through each sample and add it to the right topo node
    ElemIndex elem;
    QVector<qreal>::Iterator p;
    for(elem=0, p=this->begin; p!=this->end; elem++, p+=this->numDimensions)
    {
        //qDebug( "collectTopoSamples loop2 %d", elem);

        // Get vars
        int dse = *(p+dataSourceDim);
        int cpu = *(p+cpuDim);
        int cycles = *(p+latencyDim);
        //qDebug( "collectTopoSamples cpu= %d cpuDim= %d topo->CPUIDMap.size()=%d", cpu, cpuDim, topo->CPUIDMap.size());

        // Search for nodes
        Thread* t = threadIdMap[cpu];
        qDebug( "cxx = %p", t);


        // Update data for serving resource
        QMap<DataObject*,SampleSet>*sampleSets = (QMap<DataObject*,SampleSet>*)t->metadata["sampleSets"];
        (*sampleSets)[this].totSamples.insert(elem);
        (*sampleSets)[this].totCycles += cycles;

        if(!selectionDefined() || selected(elem))
        {
            (*sampleSets)[this].selSamples.insert(elem);
            (*sampleSets)[this].selCycles += cycles;
        }

        if(dse == -1)
            continue;

        // Go up to data source
        Component* c;
        for(c = (Component*)t; dse>0 && c->GetParent() && c->GetComponentType() != SYS_TOPO_COMPONENT_CHIP ; dse--, c=c->GetParent())
        {
            if(!selectionDefined() || selected(elem))
            {
                *(int*)(c->metadata["transactions"])+= 1;
            }
        }

        // Update data for core
        sampleSets = (QMap<DataObject*,SampleSet>*)c->metadata["sampleSets"];
        (*sampleSets)[this].totSamples.insert(elem);
        (*sampleSets)[this].totCycles += cycles;

        if(!selectionDefined() || selected(elem))
        {
            (*sampleSets)[this].selSamples.insert(elem);
            (*sampleSets)[this].selCycles += cycles;
        }
    }
}

int DataObject::parseCSVFile(QString dataFileName)
{
    // Open the file
    QFile dataFile(dataFileName);

    if (!dataFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return -1;

    // Create text stream
    QTextStream dataStream(&dataFile);
    QString line;
    QStringList lineValues;
    qint64 elemid = 0;

    // Get metadata from first line
    line = dataStream.readLine();
    this->meta = line.split(',');
    this->numDimensions = this->meta.size();

    sourceDim = this->meta.indexOf("source");
    lineDim = this->meta.indexOf("line");
    variableDim = this->meta.indexOf("variable");
    dataSourceDim = this->meta.indexOf("data_src");
    indexDim = this->meta.indexOf("index");
    latencyDim = this->meta.indexOf("latency");
    nodeDim = this->meta.indexOf("node");
    cpuDim = this->meta.indexOf("cpu");
    xDim = this->meta.indexOf("xidx");
    yDim = this->meta.indexOf("yidx");
    zDim = this->meta.indexOf("zidx");

    QVector<QString> varVec;
    QVector<QString> sourceVec;

    // Get data
    while(!dataStream.atEnd())
    {
        line = dataStream.readLine();
        lineValues = line.split(',');

        if(lineValues.size() != this->numDimensions)
        {
            std::cerr << "ERROR: element dimensions do not match metadata!" << std::endl;
            std::cerr << "At element " << elemid << std::endl;
            return -1;
        }

        // Process individual dimensions differently
        for(int i=0; i<lineValues.size(); i++)
        {
            QString tok = lineValues[i];
            if(i==variableDim)
            {
                ElemIndex uid = createUniqueID(varVec,tok);
                this->vals.push_back(uid);
                varNames.push_back(tok);
            }
            else if(i==sourceDim)
            {
                ElemIndex uid = createUniqueID(sourceVec,tok);
                this->vals.push_back(uid);
                fileNames.push_back(tok);
            }
            else if(i==dataSourceDim)
            {
                int dseVal = tok.toInt(NULL,16);
                this->vals.push_back(dseDepth(dseVal));
            }
            else
            {
                this->vals.push_back(tok.toLongLong());
            }
        }

        elemid++;
    }

    // Close and return
    dataFile.close();

    this->allocate();

    return 0;
}

void DataObject::setSelectionMode(selection_mode mode, bool silent)
{
    selMode = mode;

    if(silent)
        return;

    QString selcmd("select MODE=");
    switch(mode)
    {
        case(MODE_NEW):
            selcmd += "filter";
            break;
        case(MODE_APPEND):
            selcmd += "append";
            break;
        case(MODE_FILTER):
            selcmd += "filter";
            break;
    }
    con->log(selcmd);
}

void DataObject::calcStatistics()
{
    dimSums.resize(this->numDimensions);
    minimumValues.resize(this->numDimensions);
    maximumValues.resize(this->numDimensions);
    meanValues.resize(this->numDimensions);
    standardDeviations.resize(this->numDimensions);

    covarianceMatrix.resize(this->numDimensions*this->numDimensions);
    correlationMatrix.resize(this->numDimensions*this->numDimensions);

    qreal firstVal = *(this->begin);
    dimSums.fill(0);
    minimumValues.fill(9999999999);
    maximumValues.fill(0);
    meanValues.fill(0);
    standardDeviations.fill(0);

    // Means and combined means
    QVector<qreal>::Iterator p;
    ElemIndex elem;
    qreal x, y;

    QVector<qreal> meanXY;
    meanXY.resize(this->numDimensions*this->numDimensions);
    meanXY.fill(0);
    for(elem=0, p=this->begin; p!=this->end; elem++, p+=this->numDimensions)
    {
        for(int i=0; i<this->numDimensions; i++)
        {
            x = *(p+i);
            dimSums[i] += x;
            minimumValues[i] = std::min(x,minimumValues[i]);
            maximumValues[i] = std::max(x,maximumValues[i]);

            for(int j=0; j<this->numDimensions; j++)
            {
                y = *(p+j);
                meanXY[ROWMAJOR_2D(i,j,this->numDimensions)] += x*y;
            }
        }
    }

    // Divide by this->numElements to get mean
    for(int i=0; i<this->numDimensions; i++)
    {
        meanValues[i] = dimSums[i] / (qreal)this->numElements;
        for(int j=0; j<this->numDimensions; j++)
        {
            meanXY[ROWMAJOR_2D(i,j,this->numDimensions)] /= (qreal)this->numElements;
        }
    }

    // Covariance = E(XY) - E(X)*E(Y)
    for(int i=0; i<this->numDimensions; i++)
    {
        for(int j=0; j<this->numDimensions; j++)
        {
            covarianceMatrix[ROWMAJOR_2D(i,j,this->numDimensions)] =
                meanXY[ROWMAJOR_2D(i,j,this->numDimensions)] - meanValues[i]*meanValues[j];
        }
    }

    // Standard deviation of each dim
    for(elem=0, p=this->begin; p!=this->end; elem++, p+=this->numDimensions)
    {
        for(int i=0; i<this->numDimensions; i++)
        {
            x = *(p+i);
            standardDeviations[i] += (x-meanValues[i])*(x-meanValues[i]);
        }
    }

    for(int i=0; i<this->numDimensions; i++)
    {
        standardDeviations[i] = sqrt(standardDeviations[i]/(qreal)this->numElements);
    }

    // Correlation Coeff = cov(xy) / stdev(x)*stdev(y)
    for(int i=0; i<this->numDimensions; i++)
    {
        for(int j=0; j<this->numDimensions; j++)
        {
            correlationMatrix[ROWMAJOR_2D(i,j,this->numDimensions)] =
                    covarianceMatrix[ROWMAJOR_2D(i,j,this->numDimensions)] /
                    (standardDeviations[i]*standardDeviations[j]);
        }
    }
}

void DataObject::constructSortedLists()
{
    dimSortedLists.resize(this->numDimensions);
    for(int d=0; d<this->numDimensions; d++)
    {
        for(int e=0; e<this->numElements; e++)
        {
            struct indexedValue di;
            di.idx = e;
            di.val = at(e,d);
            dimSortedLists.at(d).push_back(di);
        }
        std::sort(dimSortedLists.at(d).begin(),dimSortedLists.at(d).end());
    }
}

qreal distanceHardware(DataObject *d, ElemSet *s1, ElemSet *s2)
{
    int cpuDepth = d->cpu->GetTopoTreeDepth();
    int dseDepth;

    // Vars
    std::vector<qreal> t1,t2;
    std::vector<qreal> t1means,t2means;
    std::vector<qreal> t1stddev,t2stddev;

    t1.resize(cpuDepth,0);
    t2.resize(cpuDepth,0);
    t1means.resize(cpuDepth,0);
    t1stddev.resize(cpuDepth,0);
    t2means.resize(cpuDepth,0);
    t2stddev.resize(cpuDepth,0);

    qreal n1 = s1->size();
    qreal n2 = s2->size();

    ElemSet::iterator it;
    qreal lat;

    // Collect s1 topo data
    for(it = s1->begin(); it != s1->end(); it++)
    {
        lat = d->at(*it,d->latencyDim);
        dseDepth = d->at(*it,d->dataSourceDim);
        t1[cpuDepth] += lat;
        t1[dseDepth] += lat;
    }

    // Collect s2 topo data
    for(it = s2->begin(); it != s2->end(); it++)
    {
        lat = d->at(*it,d->latencyDim);
        dseDepth = d->at(*it,d->dataSourceDim);
        t2[cpuDepth] += lat;
        t2[dseDepth] += lat;
    }

    // Compute means
    for(int i=0; i<cpuDepth; i++)
    {
        t1means[i] = t1.at(i) / n1;
        t2means[i] = t2.at(i) / n2;
    }

    // Compute standard deviations
    for(it = s1->begin(); it != s1->end(); it++)
    {
        lat = d->at(*it,d->latencyDim);
        dseDepth = d->at(*it,d->dataSourceDim);
        t1stddev[dseDepth] += (lat-t1means.at(dseDepth))*(lat-t1means.at(dseDepth));
    }
    for(it = s2->begin(); it != s2->end(); it++)
    {
        lat = d->at(*it,d->latencyDim);
        dseDepth = d->at(*it,d->dataSourceDim);
        t2stddev[dseDepth] += (lat-t2means.at(dseDepth))*(lat-t2means.at(dseDepth));
    }
    for(int i=0; i<cpuDepth; i++)
    {
        t1stddev[i] /= n1;
        t2stddev[i] /= n2;
    }

    // Euclidean length of means vector
    qreal dist = 0;
    for(int i=0; i<cpuDepth; i++)
    {
        dist += (t1means[i]+t2means[i])*(t1means[i]+t2means[i]);
    }
    dist = sqrt(dist);

    return dist;
}
