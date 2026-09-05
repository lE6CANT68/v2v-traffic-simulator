/**
 * @file OpenGLWidget.h
 * @brief Définition du widget OpenGL pour la simulation de voitures sur un graphe.
 *
 * Ce fichier contient la classe OpenGLWidget qui hérite de QOpenGLWidget et
 * gère le rendu de la simulation, les interactions utilisateur (souris, clavier),
 * le zoom, la caméra, ainsi que le chargement des tuiles OSM pour la carte.
 */
#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <vector>
#include <cmath>
#include "Voiture.h"
#include "Graphe.h"
#include "qnetworkaccessmanager.h"
#include <cstdlib>
#include <QSlider>
#include <QLabel>
#include <QPainter>
#include "GrapheInterference.h"

/**
 * @class OpenGLWidget
 * @brief Widget Qt pour afficher et gérer une simulation de voitures sur un graphe.
 *
 * La classe gère :
 * - Le rendu OpenGL des voitures, du graphe et des arêtes d'interférence.
 * - La caméra et le zoom.
 * - La sélection de voiture avec la souris.
 * - Le suivi d'une voiture sélectionnée.
 * - La simulation de déplacement des voitures.
 * - Le chargement des tuiles OpenStreetMap (OSM) pour la carte.
 */

class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur du widget OpenGL.
     * @param parent Pointeur vers le widget parent.
     *
     * Initialise la simulation, les sliders pour le rayon et la vitesse, et démarre le timer de mise à jour.
     */

    OpenGLWidget(QWidget* parent);


    /**
     * @brief Active ou désactive l'affichage de l'écran de chargement.
     * @param l Booléen indiquant si le chargement est actif.
     */
    void setLoading(bool l);

    /**
     * @brief Récupère le vecteur des voitures.
     * @return Référence au vecteur de Voiture.
     */
    std::vector<Voiture>& getVoitures();
    void ajouterVoiture(const Voiture& v);
    void setFacteurVitesse(double nb);
    void setVoitureSuivie(Voiture* v);
    /**
     * @brief Récupère le nombre d'arêtes d'interférence calculé.
     * C'est la passerelle publique vers GrapheInterference.
     * @return Le nombre d'arêtes d'interférence (int).
     */
    int getNombreAretesInterference() const {
        return grapheInterference.getAretes().size();
    }

    /**
     * @brief Supprime une voiture donnée de la simulation.
     * @param v Pointeur vers la voiture à supprimer.
     */
    void supprimerVoiture(Voiture* v);

    /**
     * @brief Supprime la voiture actuellement suivie.
     * @deprecated Utiliser supprimerVoiture(voitureSuivie) à la place.
     */
    void supprimerVoitureSuivie();

    /**
     * @brief Déplace toutes les voitures en fonction du facteur de vitesse et des proximités.
     */
    void deplacerVoitures();

    /**
     * @brief Met en pause ou reprend la simulation.
     */
    void togglePause();

    /**
     * @brief Charge un graphe et initialise les voitures.
     * @param pathOSM Chemin vers le fichier OSM.
     * @param nbVoitures Nombre de voitures à créer.
     */
    void chargerGrapheEtVoitures(const std::string& pathOSM, int nbVoitures);

    /**
     * @brief Détruit complètement la simulation, toutes les voitures et les données du graphe.
     */
    void detruireSimulation();

    Graphe graphe;/**< Graphe de la simulation */

protected:

    /**
     * @brief Initialisation OpenGL.
     */
    void initializeGL() override;

    /**
     * @brief Gestion du redimensionnement de la fenêtre OpenGL.
     * @param w Largeur de la fenêtre.
     * @param h Hauteur de la fenêtre.
     */
    void resizeGL(int w, int h) override;

    /**
     * @brief Fonction de rendu OpenGL.
     */
    void paintGL() override;

    /**
     * @brief Gestion des événements clavier pour déplacement de la caméra et zoom.
     * @param event Événement clavier.
     */
    void keyPressEvent(QKeyEvent* event);

    /**
     * @brief Gestion des événements souris lors d'un clic.
     * @param event Événement souris.
     */
    void mousePressEvent(QMouseEvent* event);

    /**
     * @brief Gestion des événements souris lors d'un relâchement.
     * @param event Événement souris.
     */
    void mouseReleaseEvent(QMouseEvent* event);

    /**
     * @brief Gestion de la molette de la souris pour zoom.
     * @param event Événement molette.
     */
    void mouseMoveEvent(QMouseEvent* event);

    void wheelEvent(QWheelEvent* event);

private:
    GrapheInterference grapheInterference;
    bool loading = false;   /**< Indique si l'écran de chargement est actif */
    QString loadingText = "Chargement en cours..."; /**< Texte affiché pendant le chargement */

    double minLat = 0.0;/**< Latitude minimale du graphe chargé */
    double minLon = 0.0;/**< Longitude minimale du graphe chargé */
    double maxLat = 0.0;/**< Latitude maximale du graphe chargé */
    double maxLon = 0.0; /**< Longitude maximale du graphe chargé */
    double scale = 1.0;/**< Facteur d'échelle pour convertir lat/lon en coordonnées OpenGL */

    /**
     * @struct TuileOSM
     * @brief Représente une tuile OpenStreetMap chargée.
     */
    struct TuileOSM {
        int x, y, z;        /**< Coordonnées de la tuile (x, y) et niveau de zoom z */
        QImage image;       /**< Image de la tuile téléchargée */
        GLuint textureId;   /**< Identifiant de la texture OpenGL associée */
    };

    QMap<std::tuple<int,int,int>, TuileOSM> tuilesCache; /**< Cache des tuiles OSM téléchargées */
    QNetworkAccessManager* netManager;/**< Gestionnaire de requêtes réseau pour télécharger les tuiles */
    int zoomOSM = 16;   /**< Niveau de zoom par défaut pour les tuiles */

    QPoint mousePressPos; /**< Position de la souris lors du clic initial */
    bool leftButtonPressed; /**< Indique si le bouton gauche de la souris est pressé */
    const int clickTolerance;/**< Tolérance pour détecter un drag vs un clic */
    const int selectionRadiusPx;/**< Rayon de sélection d'une voiture en pixels */

    std::vector<Voiture> voitures;/**< Liste des voitures dans la simulation */


    float camX, camY;/**< Position de la caméra */
    float zoom;/**< Niveau de zoom actuel */
    bool dragging; /**< Indique si la vue est en train d'être déplacée */
    QPoint lastMousePos;/**< Dernière position connue de la souris */
    int frameCount;/**< Compteur de frames pour les calculs périodiques */
    float caseSize;/**< Taille d'une case pour la grille d'interférence */

    Voiture* voitureSuivie;/**< Pointeur vers la voiture actuellement suivie */
    float zoomCible;/**< Zoom cible pour le suivi de la voiture */
    float vitesseZoom; /**< Vitesse de transition du zoom */
    float followLissage; /**< Lissage pour le suivi de caméra */
    double facteurVitesse = 1.0;/**< Facteur de vitesse de la simulation */
    bool simulationPaused = false;/**< Indique si la simulation est en pause */
    int lastSpeedValue = 100;/**< Dernière valeur de vitesse avant pause */
    QSlider* speedSlider;/**< Slider de contrôle de vitesse */
    QLabel* speedLabel;/**< Label affichant la vitesse actuelle */

    QLabel* infoBulle = nullptr; /**< Info-bulle affichée pour la voiture sélectionnée */
    void mettreAJourInfoBulle();
    /**
     * @brief Convertit des coordonnées latitude/longitude en coordonnées de tuile OSM.
     * @param lat Latitude en degrés
     * @param lon Longitude en degrés
     * @param zoom Niveau de zoom
     * @return Paire (x, y) correspondant aux coordonnées de tuile
     */
    std::pair<int,int> latLonToTile(double lat, double lon, int zoom);

    /**
     * @brief Télécharge et met en cache une tuile OSM spécifique.
     * @param x Coordonnée x de la tuile
     * @param y Coordonnée y de la tuile
     * @param z Niveau de zoom
     */
    void chargerTuile(int x, int y, int z);

    /**
     * @brief Dessine un cercle en OpenGL.
     * @param cx Position X du centre
     * @param cy Position Y du centre
     * @param radius Rayon du cercle
     * @param segments Nombre de segments pour approximer le cercle
     * @param color Couleur du cercle
     */
    void drawCircle(float cx, float cy, float radius, int segments, QColor color);



    /**
     * @brief Affiche l'info-bulle pour une voiture donnée.
     * @param v Pointeur vers la voiture sélectionnée
     */
    void afficherInfoBulle(Voiture* v);

    /**
     * @brief Cache l'info-bulle actuellement affichée.
     */
    void cacherInfoBulle();
signals:
    void simulationUpdated();
};

#endif // OPENGLWIDGET_H
