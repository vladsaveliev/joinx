#pragma once

#include "common/CoordinateView.hpp"

#include <boost/config.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/unordered_set.hpp>

#include <memory>
#include <vector>

// FIXME: this doesn't belong here! we'll want to generalize the
// region extraction concept and put this vcf-specific one with vcf
// stuff
#include "fileformats/vcf/RawVariant.hpp"
#include "fileformats/vcf/Entry.hpp"

struct VcfRegionExtractor {
    typedef boost::unordered_set<Region> ReturnType;

    ReturnType operator()(Vcf::Entry const& entry) const {
        auto rawvs = Vcf::RawVariant::processEntry(entry);
        ReturnType rv;
        for (auto i = rawvs.begin(); i != rawvs.end(); ++i) {
            // if we set the region begin to the region end then
            // we would be reporting only the start position.
            // that is a useful option sometimes.
            rv.insert(i->region());
        }
        // For ref-only entries:
        if (rawvs.empty()) {
            Region reg{entry.start(), entry.stop()};
            rv.insert(reg);
        }
        return rv;
    }
};
// END FIXME


template<
          typename OutputFunc
        , typename RegionExtractor = VcfRegionExtractor // FIXME: get rid of default
        >
class GroupBySharedRegions {
public:
    typedef typename RegionExtractor::ReturnType RegionSet;

    GroupBySharedRegions(
              OutputFunc& out
            , RegionExtractor regionExtractor = RegionExtractor()
            )
        : out_(out)
        , regionExtractor_(regionExtractor)
    {}

    template<typename Members, typename Graph>
    void add_component(Members const& xs, Graph& g) {
        using namespace boost;
        for (std::size_t i = 0; i < xs.size(); ++i) {
            for (std::size_t j = i + 1; j < xs.size(); ++j) {
                add_edge(xs[i], xs[j], g);
            }
        }
    }

    // Sort vectors of entries (all of which should have the same region) by
    // start/stop position
    struct SortHelper_ {
        template<typename GroupType>
        bool operator()(GroupType const& x, GroupType const& y) const {
            if (x->empty() || y->empty())
                return true; // doesn't matter if one or both are empty;

            if ((*x)[0]->start() < (*y)[0]->start())
                return true;

            if ((*x)[0]->start() > (*y)[0]->start())
                return false;

            return (*x)[0]->stop() < (*y)[0]->stop();
        }
    };

    template<typename ValuePtr>
    void operator()(std::vector<ValuePtr> entries) {
        typedef std::vector<ValuePtr> ValuePtrVector;

        std::vector<RegionSet> regions(entries.size());
        boost::unordered_map<Region, boost::unordered_set<std::size_t>> regionToEntries;
        for (std::size_t i = 0; i < entries.size(); ++i) {
            std::size_t entryIdx = i;
            regions[i] = regionExtractor_(*entries[i]);
            for (auto j = regions[i].begin(); j != regions[i].end(); ++j) {
                regionToEntries[*j].insert(entryIdx);
            }
        }

        using namespace boost;
        typedef adjacency_matrix<undirectedS> Graph;
        Graph graph(entries.size());

        for (auto x = regionToEntries.begin(); x != regionToEntries.end(); ++x) {
            std::vector<std::size_t> xs(x->second.begin(), x->second.end());
            add_component(xs, graph);
        }

        std::vector<int> components(entries.size());
        connected_components(graph, &components[0]);

        boost::unordered_map<int, ValuePtrVector> groups;
        for (std::size_t i = 0; i < components.size(); ++i) {
            groups[components[i]].push_back(std::move(entries[i]));
        }

        // The groups are not necessarily sorted at this point
        // Let's fix that...
        // gcc 4.4 can't deal with sorting std::vector<std::vector<std::unique_ptr<T>>>
        // using raw pointers instead
        std::vector<ValuePtrVector*> sortedGroups;
        sortedGroups.reserve(groups.size());
        for (auto i = groups.begin(); i != groups.end(); ++i) {
            sortedGroups.push_back(&i->second);
        }
        std::sort(sortedGroups.begin(), sortedGroups.end(), SortHelper_{});

        for (auto i = sortedGroups.begin(); i != sortedGroups.end(); ++i) {
            out_(std::move(**i));
        }
    }

private:
    OutputFunc& out_;
    RegionExtractor regionExtractor_;
};

template<
          typename OutputFunc
        , typename RegionExtractor = VcfRegionExtractor // FIXME: get rid of default
        >
GroupBySharedRegions<OutputFunc, RegionExtractor>
makeGroupBySharedRegions(
              OutputFunc& out
            , RegionExtractor regionExtractor = RegionExtractor()
            )
{
    return GroupBySharedRegions<OutputFunc, RegionExtractor>(out, regionExtractor);
}
