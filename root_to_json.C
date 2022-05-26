#include <iostream>
#include <fstream>
#include <regex>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "TAxis.h"
#include "TClass.h"
#include "TFile.h"
#include "TGraph.h"
#include "TH1.h"
#include "TH2.h"
#include "TKey.h"
#include "TObjString.h"
#include "TROOT.h"
#include "TVectorD.h"

namespace pt = boost::property_tree;

void to_pt(const TGraph &graph, pt::ptree &root, const std::string &name)
{
    pt::ptree graphTree;

    pt::ptree X;
    pt::ptree Y;

    const int nPoints = graph.GetN();
    auto fX = graph.GetX();
    auto fY = graph.GetY();

    for (int i = 0; i < nPoints; i++)
    {
        pt::ptree x, y;
        x.put_value(fX[i]);
        y.put_value(fY[i]);

        X.push_back(std::make_pair("", x));
        Y.push_back(std::make_pair("", y));
    }

    graphTree.add_child("x", X);
    graphTree.add_child("y", Y);

    root.add_child(name, graphTree);
}

template <class T>
void to_pt(const TVectorT<T> &vec, pt::ptree &root, const std::string &name)
{
    pt::ptree values;

    const int nElements = vec.GetNoElements();

    for (int i = 0; i < nElements; i++)
    {
        pt::ptree v;
        v.put_value(vec[i]);

        values.push_back(std::make_pair("", v));
    }

    root.add_child(name, values);
}

void to_pt(const TAxis &axis, pt::ptree &root, const std::string &name)
{
    pt::ptree values;

    const int nPoints = axis.GetNbins();

    for (int i = 1; i <= nPoints + 1; i++)
    {
        double edge = axis.GetBinLowEdge(i);

        pt::ptree v;
        v.put_value(edge);

        values.push_back(std::make_pair("", v));
    }

    root.add_child(name, values);
}

void to_pt(const TH1 &hist, pt::ptree &root, const std::string &name)
{
    const int nBins = hist.GetNbinsX();

    pt::ptree histTree;
    pt::ptree values;
    pt::ptree err_sq;

    for (int i = 0; i <= nBins + 1; i++)
    {
        const double err = hist.GetBinError(i);

        pt::ptree v;
        pt::ptree e;

        v.put_value(hist.GetBinContent(i));
        e.put_value(err * err);

        values.push_back(std::make_pair("", v));
        err_sq.push_back(std::make_pair("", e));
    }

    histTree.add_child("values", values);
    histTree.add_child("err_sq", err_sq);

    to_pt(*hist.GetXaxis(), histTree, "bins");

    root.add_child(name, histTree);
}

void to_pt(const TH2 &hist, pt::ptree &root, const std::string &name)
{
    const int nBinsX = hist.GetNbinsX();
    const int nBinsY = hist.GetNbinsY();

    pt::ptree histTree, values, err_sq;

    for (int i = 0; i <= nBinsX + 1; i++)
    {
        pt::ptree values_row, err_sq_row;

        for (int j = 0; j <= nBinsY + 1; j++)
        {
            const int binIdx = hist.GetBin(i, j);

            pt::ptree v, e;
            const double err = hist.GetBinError(binIdx);

            v.put_value(hist.GetBinContent(binIdx));
            e.put_value(err * err);

            values_row.push_back(std::make_pair("", v));
            err_sq_row.push_back(std::make_pair("", e));
        }

        values.push_back(std::make_pair("", values_row));
        err_sq.push_back(std::make_pair("", err_sq_row));
    }

    histTree.add_child("values", values);
    histTree.add_child("err_sq", err_sq);

    to_pt(*hist.GetXaxis(), histTree, "bins_x");
    to_pt(*hist.GetYaxis(), histTree, "bins_y");

    root.add_child(name, histTree);
}

void to_pt(const TString &str, pt::ptree &root, const std::string &name)
{
    root.put(name, std::string(str.View()));
}

void to_pt(const TObjString &str, pt::ptree &root, const std::string &name)
{
    to_pt(str.GetString(), root, name);
}

void to_pt(TDirectory &dir, pt::ptree &root, const std::string &name)
{
    pt::ptree dirTree;
    std::cerr << ".";

    TKey *key;
    TIter next_key(dir.GetListOfKeys());

    while ((key = static_cast<TKey*>(next_key())))
    {
        TClass *cl = gROOT->GetClass(key->GetClassName());
        if (cl == nullptr) {
            continue;
        }

        const std::string objName = key->GetName();

        if (cl->InheritsFrom(TDirectory::Class()))
        {
            TDirectory *subdir = dir.GetDirectory(objName.c_str());
            to_pt(*subdir, dirTree, objName);
        }
        else if (cl->InheritsFrom(TH2::Class()))
        {
            auto *hist = static_cast<TH2*>(dir.Get(objName.c_str()));
            to_pt(*hist, dirTree, objName);
        }
        else if (cl->InheritsFrom(TH1::Class()))
        {
            auto *hist = static_cast<TH1*>(dir.Get(objName.c_str()));
            to_pt(*hist, dirTree, objName);
        }
        else if (cl->InheritsFrom(TObjString::Class()))
        {
            auto *str = static_cast<TObjString*>(dir.Get(objName.c_str()));
            to_pt(*str, dirTree, objName);
        }
        else if (cl->InheritsFrom(TVectorD::Class()))
        {
            auto *vec = static_cast<TVectorD*>(dir.Get(objName.c_str()));
            to_pt(*vec, dirTree, objName);
        }
        else if (cl->InheritsFrom(TGraph::Class()))
        {
            auto *graph = static_cast<TGraph*>(dir.Get(objName.c_str()));
            to_pt(*graph, dirTree, objName);
        }
    }

    root.add_child(name, dirTree);
}

pt::ptree parse_input_file(const std::string &fname)
{
    pt::ptree root;

    TFile inFile(fname.c_str());
    to_pt(inFile, root, "root");

    return root;
}

void write_json(const std::string &fname, const pt::ptree &root)
{
    std::ofstream s(fname);

    /*
     * NOTE: we dump json to to string first in order to remove quotes around
     *       numbers that boost serialized as strings.
     */
    std::stringstream ss;
    ss << std::setprecision(std::numeric_limits<double>::max_digits10);
    pt::write_json(ss, root.get_child("root"), false);

    const std::string json = ss.str();

    std::regex reQuotedNum("\"(-?[0-9]*(?:\\.[0-9]+)?(?:e[\\+-][0-9]+)?)\"");

    std::regex_replace(
        std::ostreambuf_iterator<char>(s),
        json.begin(), json.end(), reQuotedNum, "$1"
    );
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: root_to_json INPUT OUTPOUT" << std::endl;
        return 1;
    }

    const std::string input (argv[1]);
    const std::string output(argv[2]);

    std::cout << "Starting..." << std::endl;
    std::cout << "  Input  File: " << input << std::endl;
    std::cout << "  Output File: " << output << std::endl;

    std::cout << "  Exporting: ";
    const pt::ptree root = parse_input_file(input);

    std::cout << std::endl << "  Writing output file..." << std::endl;
    write_json(output, root);

    std::cout << "Done." << std::endl;
    return 0;
}

