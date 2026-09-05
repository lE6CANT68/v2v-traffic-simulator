#ifndef GRAPHEINTERFERENCE_H
#define GRAPHEINTERFERENCE_H

#include <vector>
#include <unordered_map>
#include "Voiture.h"

class CaseIndex {
public:
    int x;
    int y;

    CaseIndex(int xx = 0, int yy = 0) : x(xx), y(yy) {}

    bool operator==(const CaseIndex& other) const {
        return x == other.x && y == other.y;
    }

    // Hashcode d une case
    size_t hash() const {
        return std::hash<int>()(x) ^ (std::hash<int>()(y) << 1);//Bordel
    }
};

// Hash fonction pour unordered_map
class CaseIndexHasher {
public:
    size_t operator()(const CaseIndex& ci) const {
        return ci.hash();
    }
};

class GrapheInterference {
public:
    GrapheInterference(float tailleCase = 2 * Voiture::RAYON_INTERFERENCE)
        : caseSize(tailleCase) {}

    void calculer(const std::vector<Voiture>& voitures,
                  float camX, float camY,
                  float screenWidth, float screenHeight,
                  float zoom);

    const std::vector<std::pair<int,int>>& getAretes() const { return aretesVisibles; }
    void clear() { aretesVisibles.clear(); }
    void setCaseSize(float size);
    void ajouterArete(int idx1, int idx2) ;
    int nbArretesInterference()const {return aretesVisibles.size();};

private:
    float caseSize;
    std::vector<std::pair<int,int>> aretesVisibles;

};

#endif // GRAPHEINTERFERENCE_H
