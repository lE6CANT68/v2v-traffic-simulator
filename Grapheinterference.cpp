/**
 * @file GrapheInterference.cpp
 * @brief Calcul des arêtes d'interférence entre voitures pour la simulation.
 *
 * Cette classe permet de détecter quelles voitures sont proches les unes des autres
 * et donc susceptibles de se gêner (zone d'interférence), en tenant compte du viewport.
 */

#include "GrapheInterference.h"
#include <cmath>

/**
 * @brief Calcule les arêtes d'interférence visibles entre les voitures
 * @param voitures Vecteur des voitures présentes dans la simulation
 * @param camX Coordonnée X du coin supérieur gauche de la caméra
 * @param camY Coordonnée Y du coin supérieur gauche de la caméra
 * @param screenWidth Largeur de l'écran en pixels
 * @param screenHeight Hauteur de l'écran en pixels
 * @param zoom Facteur de zoom de la caméra
 *
 * La fonction utilise une grille spatiale pour limiter le nombre de comparaisons
 * et ne teste que les voitures dans les cases voisines.
 * Les paires de voitures dont la distance est inférieure au double du
 * rayon d'interférence sont ajoutées à aretesVisibles.
 */
void GrapheInterference::calculer(const std::vector<Voiture>& voitures,
                                  float camX, float camY,
                                  float screenWidth, float screenHeight,
                                  float zoom)
{
    // Réinitialiser la liste des arêtes visibles
    aretesVisibles.clear();

    if (voitures.empty()) return;

    float invZoom = 1.0f / zoom;

    // Définir la zone visible
    float xMin = camX;
    float yMin = camY;
    float xMax = camX + screenWidth * invZoom;
    float yMax = camY + screenHeight * invZoom;

    // Grille spatiale : chaque case contient indices de voitures
    std::unordered_map<CaseIndex, std::vector<int>, CaseIndexHasher> grid;

    int n = voitures.size();
    for (int i = 0; i < n; ++i) {
        const Voiture& v = voitures[i];
        float vx = v.getX();
        float vy = v.getY();

        // Si voiture hors viewport (avec marge = rayon d'interférence), on ignore
        if (vx + Voiture::RAYON_INTERFERENCE < xMin ||
            vx - Voiture::RAYON_INTERFERENCE > xMax ||
            vy + Voiture::RAYON_INTERFERENCE < yMin ||
            vy - Voiture::RAYON_INTERFERENCE > yMax)
            continue;

        int cx = int(std::floor(vx / caseSize));
        int cy = int(std::floor(vy / caseSize));
        CaseIndex ci(cx, cy);
        grid[ci].push_back(i);
    }

    // Pour chaque case, tester uniquement les voitures dans les 9 cases voisines
    for (auto& pair : grid) {
        CaseIndex ci = pair.first;
        const std::vector<int>& voituresCell = pair.second;

        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                CaseIndex voisin(ci.x + dx, ci.y + dy);

                auto it = grid.find(voisin);
                if (it == grid.end()) continue;

                const std::vector<int>& voituresVoisin = it->second;

                // Comparer toutes les paires entre la case et la voisine
                for (size_t i = 0; i < voituresCell.size(); ++i) {
                    int idx1 = voituresCell[i];
                    for (size_t j = 0; j < voituresVoisin.size(); ++j) {
                        int idx2 = voituresVoisin[j];
                        if (idx1 >= idx2) continue; // éviter doublons

                        float dx = voitures[idx1].getX() - voitures[idx2].getX();
                        float dy = voitures[idx1].getY() - voitures[idx2].getY();
                        float dist = std::sqrt(dx*dx + dy*dy);

                        // Ajouter l'arête si les voitures sont proches
                        if (dist <= 2 * Voiture::RAYON_INTERFERENCE)
                            aretesVisibles.push_back({idx1, idx2});
                    }
                }
            }
        }
    }

}
void GrapheInterference::setCaseSize(float size){
    caseSize = size;
}
void GrapheInterference::ajouterArete(int idx1, int idx2) {
    aretesVisibles.push_back({idx1, idx2});
}
