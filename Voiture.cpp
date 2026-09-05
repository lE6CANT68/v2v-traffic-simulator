/**
 * @file Voiture.cpp
 * @brief Implémentation de la classe Voiture pour la simulation de trafic.
 */

#include "Voiture.h"
#include <cmath>
#include <cstdlib>
#include <QLabel>
#include <QColor>
#include <QFont>

/** @brief Longueur standard d'une voiture en unités de simulation */
const int Voiture::LONGUEUR_VOITURE = 12;

/** @brief Largeur standard d'une voiture en unités de simulation */
const int Voiture::LARGEUR_VOITURE = 7;

/** @brief Rayon d'interférence pour détecter les collisions avec d'autres voitures */
double Voiture::RAYON_INTERFERENCE = 10.0;

/** @brief Temps par frame (60 FPS ~ 16 ms) */
static constexpr double dt = 0.016;

/**
 * @brief Constructeur de la voiture
 * @param id Identifiant unique de la voiture
 * @param depart Noeud de départ
 * @param vitesse Vitesse initiale
 */
Voiture::Voiture(int id, Noeud* depart, double vitesse)
    : d_id(id), d_currentNode(depart), d_vitesse(vitesse), d_ancienNoeud(nullptr), d_vitesseReelle(0.0),
    infoBulle(nullptr), framesBloquee(0), d_forceMove(false)
{
    d_x = depart->getX();
    d_y = depart->getY();
    choisirProchainVoisin();
    d_couleur = QColor(rand()%256, rand()%256, rand()%256);
}

/**
 * @brief Déplace la voiture en fonction de la vitesse et des voitures proches
 * @param facteurVitesse Facteur de vitesse pour ajuster le déplacement
 * @param voituresProches Liste des voitures proches pour éviter les collisions
 */
void Voiture::deplacer(double facteurVitesse, const std::vector<Voiture*>& voituresProches)
{
    if (!d_currentNode || !d_nextNode) return;

    // direction vers next node
    double deltaXVersNoeud = d_nextNode->getX() - d_x;
    double deltaYVersNoeud = d_nextNode->getY() - d_y;
    double distanceVersNoeud = std::sqrt(deltaXVersNoeud * deltaXVersNoeud + deltaYVersNoeud * deltaYVersNoeud);
    if (distanceVersNoeud < 1e-6) return;

    double directionX = deltaXVersNoeud / distanceVersNoeud;
    double directionY = deltaYVersNoeud / distanceVersNoeud;

    double vitesseEffective = d_vitesse * facteurVitesse;

    const double distanceSecurite = 0.5 * Voiture::RAYON_INTERFERENCE;
    const double seuilCollision = Voiture::LONGUEUR_VOITURE;

    // Ajustement de la vitesse si voitures proches
    if (!d_forceMove) {
        for (auto* autreVoiture : voituresProches) {
            double dxAutre = autreVoiture->getX() - d_x;
            double dyAutre = autreVoiture->getY() - d_y;
            double distAutre = std::sqrt(dxAutre*dxAutre + dyAutre*dyAutre);
            if (distAutre < 1e-6) distAutre = 1e-6; // éviter division par 0

            double projectionAvant = dxAutre * directionX + dyAutre * directionY;

            double dirAutreX = autreVoiture->d_nextNode->getX() - autreVoiture->getX();
            double dirAutreY = autreVoiture->d_nextNode->getY() - autreVoiture->getY();
            double normeDirAutre = std::sqrt(dirAutreX*dirAutreX + dirAutreY*dirAutreY);
            if (normeDirAutre > 1e-6) { dirAutreX /= normeDirAutre; dirAutreY /= normeDirAutre; }

            double alignement = directionX * dirAutreX + directionY * dirAutreY;

            if (distAutre < distanceSecurite) {
                if (projectionAvant > 0) {
                    double facteurFrein = distAutre / distanceSecurite;
                    if (facteurFrein < 0.2) facteurFrein = 0.2;
                    vitesseEffective *= facteurFrein;
                } else if (projectionAvant < 0 && alignement < -0.8 && distAutre < seuilCollision) {
                    vitesseEffective *= 0.8;
                } else if (std::abs(projectionAvant) < 1e-6) {
                    auto voisins = d_currentNode->getVoisins();
                    if (!voisins.empty()) {
                        Noeud* oldNext = d_nextNode;
                        while (voisins.size() > 1 && voisins[rand() % voisins.size()] == oldNext) {}
                        d_nextNode = voisins[rand() % voisins.size()];
                    }
                }
            }
        }
    }

    // Déplacement réel
    double deltaX = vitesseEffective * directionX;
    double deltaY = vitesseEffective * directionY;
    double distanceParcourue = std::sqrt(deltaX*deltaX + deltaY*deltaY);
    d_vitesseReelle = distanceParcourue / dt;

    d_x += deltaX;
    d_y += deltaY;

    // Détection blocage
    if (distanceParcourue < 0.1) {
        framesBloquee++;
        if (framesBloquee > 50) d_forceMove = true;
    } else {
        framesBloquee = 0;
        d_forceMove = false;
    }

    // Changement de noeud
    if (distanceVersNoeud < vitesseEffective) {
        d_ancienNoeud = d_currentNode;
        d_x = d_nextNode->getX();
        d_y = d_nextNode->getY();
        d_currentNode = d_nextNode;
        choisirProchainVoisin();
        framesBloquee = 0;
        d_forceMove = false;
    }
}

/**
 * @brief Choisit le prochain noeud vers lequel se diriger
 */
void Voiture::choisirProchainVoisin()
{
    const auto& voisins = d_currentNode->getVoisins();
    if (voisins.empty()) {
        d_nextNode = nullptr;
        return;
    }

    if (voisins.size() == 1) {
        d_nextNode = voisins[0];
        return;
    }

    Noeud* choisi = nullptr;
    for (int i = 0; i < 10; ++i) {
        Noeud* cand = voisins[rand() % voisins.size()];
        if (cand != d_ancienNoeud) {
            choisi = cand;
            break;
        }
    }

    if (!choisi) {
        choisi = voisins[rand() % voisins.size()];
    }

    d_nextNode = choisi;
}

/**
 * @brief Crée ou met à jour la QLabel affichant les informations de la voiture
 * @param parent Widget parent pour la QLabel
 * @return QLabel* Pointeur vers l'info-bulle
 */
QLabel* Voiture::getInfoBulle(QWidget* parent) {
    if (!infoBulle) {
        infoBulle = new QLabel(parent);
        infoBulle->setStyleSheet("background-color: yellow; border: 1px solid black;");
        infoBulle->setAlignment(Qt::AlignCenter);
        infoBulle->setFont(QFont("Arial", 10));
        infoBulle->setFixedSize(120, 40);
        infoBulle->show();
    }
    infoBulle->setText(QString("Voiture %1\nVitesse: %2 km/h").arg(d_id).arg(int(getVitesseKmH())));
    infoBulle->move(int(d_x), int(parent->height() - d_y - infoBulle->height()));
    return infoBulle;
}

/**
 * @brief Cache et détruit l'info-bulle de la voiture
 */
void Voiture::cacherInfoBulle() {
    if (infoBulle) {
        infoBulle->hide();
        infoBulle->deleteLater();
        infoBulle = nullptr;
    }
}

/**
 * @brief Vérifie si l'info-bulle est visible
 * @return true si visible, false sinon
 */
bool Voiture::isInfoBulleVisible() const {
    return infoBulle && infoBulle->isVisible();
}

/** @brief Modifie la vitesse de la voiture */
void Voiture::setVitesse(int v) { d_vitesse = v; }

/** @brief Retourne la vitesse programmée de la voiture */
int Voiture::getVitesse() const { return d_vitesse; }

/** @brief Retourne l'identifiant de la voiture */
int Voiture::getId() const { return d_id; }

/** @brief Retourne la position X de la voiture */
int Voiture::getX() const { return d_x; }

/** @brief Retourne la position Y de la voiture */
int Voiture::getY() const { return d_y; }

/** @brief Retourne la couleur de la voiture */
QColor Voiture::getCouleur() const { return d_couleur; }

/** @brief Modifie la position X de la voiture */
void Voiture::setX(double x) { d_x = x; }

/** @brief Modifie la position Y de la voiture */
void Voiture::setY(double y) { d_y = y; }

/** @brief Retourne la vitesse réelle en km/h */
double Voiture::getVitesseKmH() const {
    return d_vitesseReelle * 3.6;
}
