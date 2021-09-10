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

#include "hwtopo.h"

#include <iostream>
using namespace std;

// hwNode::hwNode()
// {
// }
//
// hwTopo::hwTopo()
// {
//     numCPUs = 0;
//     numNUMADomains = 0;
//     totalDepth = 0;
//
//     hardwareResourceRoot = NULL;
// }
//
// hwNode *hwTopo::hardwareResourceNodeFromXMLNode(QXmlStreamReader *xml, hwNode *parent)
// {
//     hwNode *newLevel = new hwNode();
//
//     newLevel->parent = parent;
//     if(xml->name() == "object" && xml->attributes().hasAttribute("type"))
//         newLevel->name = xml->attributes().value("type").toString(); // Machine, Package, NUMANode, Cache, Core, PU
//     newLevel->depth = (parent == NULL) ? 0 : parent->depth + 1;
//
//     totalDepth = max(totalDepth, newLevel->depth);
//
//     newLevel->size = 0;
//     if(xml->attributes().hasAttribute("cache_size"))
//         newLevel->size = xml->attributes().value("cache_size").toString().toLongLong();
//     if(xml->attributes().hasAttribute("local_memory"))
//         newLevel->size = xml->attributes().value("local_memory").toString().toLongLong();
//
//
//     if(xml->attributes().hasAttribute("depth")) //depth for Cache level
//         newLevel->id = xml->attributes().value("depth").toString().toLongLong();
//     if(xml->attributes().hasAttribute("os_index")) //os_index for Machine, Package, NUMANode, Core, PU
//         newLevel->id = xml->attributes().value("os_index").toString().toLongLong();
//     //qDebug( "---hardwareResourceNodeFromXMLNode_new    adding %s  , %d (%lld)", qUtf8Printable(newLevel->name), newLevel->id,  newLevel->size);
//
//     // Read and add children if existent
//     xml->readNext();
//
//     while(!(xml->isEndElement() && xml->name() == "object"))
//     {
//         if(xml->isStartElement() && xml->name() == "object")
//         {
//             hwNode *child = hardwareResourceNodeFromXMLNode(xml, newLevel);
//             newLevel->children.push_back(child);
//         }
//
//         xml->readNext();
//     }
//     return newLevel;
// }
//
// hwNode *hwTopo::hardwareResourceNodeFromXMLNode_old(QXmlStreamReader *xml, hwNode *parent)
// {
//     qDebug( "---hardwareResourceNodeFromXMLNode_old");
//
//     hwNode *newLevel = new hwNode();
//
//     newLevel->parent = parent;
//     newLevel->name = xml->name().toString(); // Hardware, NUMA, Cache, CPU
//     newLevel->depth = (parent == NULL) ? 0 : parent->depth + 1;
//
//     totalDepth = max(totalDepth, newLevel->depth);
//
//     if(xml->attributes().hasAttribute("id"))
//         newLevel->id = xml->attributes().value("id").toString().toLongLong();
//
//     if(xml->attributes().hasAttribute("size"))
//         newLevel->size = xml->attributes().value("size").toString().toLongLong();
//
//     // Read and add children if existent
//     xml->readNext();
//
//     while(!(xml->isEndElement() && xml->name() == newLevel->name))
//     {
//         if(xml->isStartElement())
//         {
//             hwNode *child = hardwareResourceNodeFromXMLNode(xml, newLevel);
//             newLevel->children.push_back(child);
//         }
//
//         xml->readNext();
//     }
//
//     return newLevel;
// }
//
// int hwTopo::loadHardwareTopologyFromXML(QString fileName)
// {
//     QFile* file = new QFile(fileName);
//     qDebug( "loadHardwareTopologyFromXML fileName %s ", fileName.toStdString().c_str());
//
//     if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
//         return -1;
//     }
//
//     QXmlStreamReader xml(file);
//     while(!xml.atEnd() && !xml.hasError())
//     {
//         xml.readNext();
//         if(xml.isStartDocument())
//             continue;
//
//         if(xml.isStartElement())
//             if(xml.name() == "object" && xml.attributes().hasAttribute("type") && xml.attributes().value("type").toString() == "Machine")
//                 hardwareResourceRoot = hardwareResourceNodeFromXMLNode(&xml,NULL);
//
//         if(xml.isStartElement())
//             if(xml.name() == "Hardware")
//                 hardwareResourceRoot = hardwareResourceNodeFromXMLNode_old(&xml,NULL);
//     }
//
//     if(hardwareResourceRoot == NULL)
//             qFatal("No hardware topology could be loaded. Topology file: %s", fileName.toStdString().c_str());
//
//     if(xml.hasError()) {
//         return -1;
//     }
//
//     file->close();
//     xml.clear();
//
//     processLoadedTopology();
//
//     return 0;
// }
//
// void hwTopo::processLoadedTopology()
// {
//     allHardwareResourceNodes.clear();
//     hardwareResourceMatrix.clear();
//
//     CPUNodes = NULL;
//     NUMANodes = NULL;
//
//     CPUIDMap.clear();
//     NUMAIDMap.clear();
//
//     // Machine = 0 -> the whole system
//     // Package = 1 -> CPU
//     // Cache L3= 2
//     // NUMANode = 3 -> NUMA region, contains memory
//     // Cache L2 = 4
//     // Cache L1 = 5
//     // Core = 6 -> HW core
//     // PU = 7 -> processor unit (HT)
//
//     constructHardwareResourceMatrix();
//     qDebug( "loadHardwareTopologyFromXML4. depth %d ", totalDepth);
//
//     for(int i=0; i<= totalDepth; i++)
//     {
//         if(hardwareResourceMatrix[i][0]->name.contains("NUMA", Qt::CaseInsensitive))
//         {
//             numaDepth = i;
//             NUMANodes = &hardwareResourceMatrix[i];
//         }
//
//         if(hardwareResourceMatrix[i][0]->name.contains("CPU", Qt::CaseInsensitive) ||
//                 hardwareResourceMatrix[i][0]->name.contains("PU", Qt::CaseInsensitive))
//         {
//             cpuDepth = i;
//             CPUNodes = &hardwareResourceMatrix[i];
//         }
//
//     }
//     if(NUMANodes == NULL || CPUNodes == NULL)
//         qFatal("NUMANodes == NULL || CPUNodes == NULL");
//
//     numNUMADomains = NUMANodes->size();
//     numCPUs = CPUNodes->size();
//
//     qDebug( "loadHardwareTopologyFromXML4.3fileName ");
//
//     // Map NUMA IDs to NUMA Nodes
//     for(int i=0; i<NUMANodes->size(); i++)
//         NUMAIDMap[NUMANodes->at(i)->id] = NUMANodes->at(i);
//
//     // Map CPU IDs to CPU Nodes
//     for(int i=0; i<CPUNodes->size(); i++)
//         CPUIDMap[CPUNodes->at(i)->id] = CPUNodes->at(i);
// }
//
// void hwTopo::processLoadedTopology_old()
// {
//     allHardwareResourceNodes.clear();
//     hardwareResourceMatrix.clear();
//
//     CPUNodes = NULL;
//     NUMANodes = NULL;
//
//     CPUIDMap.clear();
//     NUMAIDMap.clear();
//
//     constructHardwareResourceMatrix();
//
//     NUMANodes = &hardwareResourceMatrix[1];
//     CPUNodes = &hardwareResourceMatrix[totalDepth];
//
//     numNUMADomains = NUMANodes->size();
//     numCPUs = CPUNodes->size();
//
//     // Map NUMA IDs to NUMA Nodes
//     for(int i=0; i<NUMANodes->size(); i++)
//         NUMAIDMap[NUMANodes->at(i)->id] = NUMANodes->at(i);
//
//     // Map CPU IDs to CPU Nodes
//     for(int i=0; i<CPUNodes->size(); i++)
//         CPUIDMap[CPUNodes->at(i)->id] = CPUNodes->at(i);
//
// }
//
// void hwTopo::addToMatrix(hwNode *node)
// {
//     hardwareResourceMatrix[node->depth].push_back(node);
//     allHardwareResourceNodes.push_back(node);
//
//     if(node->children.empty())
//         return;
//
//     for(int i=0; i<node->children.size(); i++)
//     {
//         addToMatrix(node->children[i]);
//     }
//
// }
//
// void hwTopo::constructHardwareResourceMatrix()
// {
//    hardwareResourceMatrix.resize(totalDepth+1);
//    addToMatrix(hardwareResourceRoot);
// }
