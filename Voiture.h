#ifndef VOITURE_H
#define VOITURE_H

#include "Noeud.h"
#include <cmath>
#include <cstdlib>
#include <QString>
#include <QFont>
#include <QPalette>
#include <QLabel>
#include <QColor>

class Voiture
{
public:
    const static int LONGUEUR_VOITURE;
    const static int LARGEUR_VOITURE;

    static double RAYON_INTERFERENCE;


    Voiture(int id, Noeud* depart, double vitesse);


    void deplacer(double facteurVitesse, const std::vector<Voiture*>& voituresProches);




    // Getters & setters
    int getX() const;
    int getY() const;
    void setX(double x) ;
    void setY(double y) ;
    int getVitesse() const;
    int getId() const;
    void setVitesse(int v);
    QColor getCouleur()const;
    double getVitesseKmH() const;

    //methode d'infobulle
    void afficherInfoBulle(QWidget* parent);
    void cacherInfoBulle();
    bool isInfoBulleVisible() const ;
     QLabel* getInfoBulle(QWidget* parent);









private:
    void choisirProchainVoisin();

    int d_id;
    double d_x, d_y;
    double d_vitesse;
    double d_vitesseReelle;
    Noeud* d_currentNode;
    Noeud* d_nextNode;
    Noeud* d_ancienNoeud;
    QColor d_couleur;

    QLabel* infoBulle ;

    bool d_forceMove ;
    int framesBloquee ;


};

#endif // VOITURE_H
