/**
 *  @file   larpandora/LArPandoraAnalysis/DoubleCountSanityCheck_module.cc
 *
 *  @brief  A module to sanity check possible double counting of hits
 */

#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Core/EDAnalyzer.h"

#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/Hit.h"

#include <string>

//------------------------------------------------------------------------------------------------------------------------------------------

namespace lar_pandora
{

/**
 *  @brief  DoubleCountSanityCheck class
 */
class DoubleCountSanityCheck : public art::EDAnalyzer
{
public:
    typedef art::Handle< std::vector<recob::PFParticle> > PFParticleHandle;
    typedef art::Handle< std::vector<recob::Cluster> > ClusterHandle;
    typedef art::Handle< std::vector<recob::Hit> > HitHandle;

    /**
     *  @brief  Constructor
     *
     *  @param  pset the input set of fhicl parameters
     */
    DoubleCountSanityCheck(fhicl::ParameterSet const &pset);

    /**
     *  @brief  Analyze an event!
     *
     *  @param  evt the art event to analyze
     */
    void analyze(const art::Event &evt);

private:

    std::string m_pandoraLabel;     ///< The label for the pandora producer
    std::string m_hitLabel;         ///< The label for the hit producer
};

DEFINE_ART_MODULE(DoubleCountSanityCheck)

} // namespace lar_pandora

//------------------------------------------------------------------------------------------------------------------------------------------
// implementation follows

#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include <iostream>
#include <unordered_map>

namespace lar_pandora
{

DoubleCountSanityCheck::DoubleCountSanityCheck(fhicl::ParameterSet const &pset) : art::EDAnalyzer(pset),
    m_pandoraLabel(pset.get<std::string>("PandoraLabel")),
    m_hitLabel(pset.get<std::string>("HitLabel"))
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

void DoubleCountSanityCheck::analyze(const art::Event &evt)
{
    // Collect the PFParticles from the event
    PFParticleHandle pfParticleHandle;
    evt.getByLabel(m_pandoraLabel, pfParticleHandle);
    
    // Collect the Clusters from the event
    ClusterHandle clusterHandle;
    evt.getByLabel(m_pandoraLabel, clusterHandle);
    
    // Collect the Hits from the event
    HitHandle hitHandle;
    evt.getByLabel(m_hitLabel, hitHandle);

    // Get the association from PFParticle->Cluster
    art::FindManyP< recob::Cluster > pfParticleToClusterAssoc(pfParticleHandle, evt, m_pandoraLabel);
    
    // Get the association from Cluster->Hit
    art::FindManyP< recob::Hit > clusterToHitAssoc(clusterHandle, evt, m_pandoraLabel);
 
    // Get the mapping from hits to PFParticles
    std::unordered_map< art::Ptr<recob::Hit>, std::vector< art::Ptr<recob::PFParticle> > > hitToPFParticlesMap;
    for (unsigned int i = 0; i < pfParticleHandle->size(); ++i)
    {
        const art::Ptr<recob::PFParticle> pPFParticle(pfParticleHandle, i);

        for (const auto &pCluster : pfParticleToClusterAssoc.at(pPFParticle.key()))
        {
            for (const auto &pHit : clusterToHitAssoc.at(pCluster.key()))
            {
                // If required add a new entry to the map for the Hit, and add this PFParticle
                hitToPFParticlesMap[pHit].push_back(pPFParticle);
            }
        }
    }

    // Now check the number of PFParticles to which each hit is associated
    const unsigned int nHits = hitHandle->size();
    unsigned int nMultiAssociatedHits = 0u;
    for (unsigned int i = 0; i < nHits; ++i)
    {
        const art::Ptr<recob::Hit> pHit(hitHandle, i);

        // Get the associated PFParticles, making an empty entry if none exist
        const auto associatedPFParticles = hitToPFParticlesMap[pHit];
        const auto nPFParticles = associatedPFParticles.size();

        std::cout << "Hit " << pHit.key() << " - nPFParticles = " << nPFParticles << std::endl;

        if (nPFParticles <= 1)
            continue;

        nMultiAssociatedHits++;
    }

    std::cout << "Of " << nHits << ", " << nMultiAssociatedHits << " were associated to more than one PFParticle" << std::endl;

    if (nMultiAssociatedHits != 0)
        throw std::logic_error("Found hits with multiple associated PFParticles!");
   
}

} //namespace lar_pandora
