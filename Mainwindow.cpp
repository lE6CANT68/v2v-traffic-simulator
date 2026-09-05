/**
 * @file MainWindow.cpp
 * @brief Fenêtre principale de l'application de simulation de trafic.
 *
 * Cette classe crée l'interface utilisateur principale avec :
 * - un panneau latéral pour les cartes, la simulation et les statistiques,
 * - un widget OpenGL pour l'affichage de la carte et des véhicules,
 * - la gestion du chargement de fichiers OSM locaux ou via Overpass API,
 * - les interactions avec les véhicules (ajout, suppression, suivi).
 */

#include "MainWindow.h"
#include "OpenGLWidget.h"
#include "qapplication.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QFileDialog>
#include <QFile>
#include <QLabel>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QInputDialog>

/**
 * @brief Constructeur de la fenêtre principale
 * @param parent Widget parent (optionnel)
 *
 * Initialise l'interface avec :
 * - la barre latérale pour carte, simulation et statistiques
 * - le widget OpenGL pour l'affichage
 * - les connexions des boutons pour les actions utilisateur
 */
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {

    // --- Création du layout principal ---
    QWidget* central = new QWidget(this);
    QVBoxLayout* globalLayout = new QVBoxLayout(central);
    QHBoxLayout* topLayout = new QHBoxLayout();

    // --- Colonne gauche ---
    QVBoxLayout* sideLayout = new QVBoxLayout();

    // --- Box carte ---
    QGroupBox* fileBox = new QGroupBox("Carte");
    QVBoxLayout* fileLayout = new QVBoxLayout();
    QPushButton* btnLoadMap = new QPushButton("Charger carte");
    QPushButton* btnLoadMapCoord = new QPushButton("Charger carte avec coordonnées");
    //QPushButton* btnLoadScenario = new QPushButton("Charger scénario");
    //QPushButton* btnSaveScenario = new QPushButton("Sauvegarder scénario");
    fileLayout->addWidget(btnLoadMap);
    fileLayout->addWidget(btnLoadMapCoord);
    //fileLayout->addWidget(btnLoadScenario);
    //fileLayout->addWidget(btnSaveScenario);
    fileBox->setLayout(fileLayout);

    // --- Box simulation ---
    QGroupBox* simBox = new QGroupBox("Simulation");
    QVBoxLayout* simLayout = new QVBoxLayout();
    QPushButton* btnPlayPause = new QPushButton("Play / Pause");
    QPushButton* btnAddVeh = new QPushButton("Ajouter véhicule");
    QPushButton* btnDelVeh = new QPushButton("Supprimer véhicule");
    simLayout->addWidget(btnPlayPause);
    simLayout->addWidget(btnAddVeh);
    simLayout->addWidget(btnDelVeh);
    simBox->setLayout(simLayout);

    // --- Box statistiques ---
    QGroupBox* statBox = new QGroupBox("Statistiques");
    QVBoxLayout* statLayout = new QVBoxLayout();
    statVehicles = new QLabel("Véhicules : 0");
    statEdges    = new QLabel("Arêtes : 0");
    statDegree   = new QLabel("Aretes interférence : 0");
    statLayout->addWidget(statVehicles);
    statLayout->addWidget(statEdges);
    statLayout->addWidget(statDegree);
    statBox->setLayout(statLayout);

    sideLayout->addWidget(fileBox);
    sideLayout->addWidget(simBox);
    sideLayout->addWidget(statBox);
    sideLayout->addStretch();
    // --- Image en bas à gauche ---
    QLabel* logo = new QLabel();
    QPixmap pix(":/images/fst.png");

    if (pix.isNull()) {
        qDebug() << "Impossible de charger fst.png";
    }

    pix = pix.scaledToWidth(200, Qt::SmoothTransformation);
    logo->setPixmap(pix);
    logo->setAlignment(Qt::AlignCenter);

    QVBoxLayout* logoLayout = new QVBoxLayout();
    logoLayout->addWidget(logo);
    logoLayout->setContentsMargins(0, 0, 0, 20);
    sideLayout->addLayout(logoLayout);

    // --- Widget OpenGL ---
    glWidget = new OpenGLWidget(this);

    topLayout->addLayout(sideLayout);
    topLayout->addWidget(glWidget, 1);
    globalLayout->addLayout(topLayout, 1);
    setCentralWidget(central);

    // --- Connexions boutons ---

    /// Play/Pause simulation
    connect(btnPlayPause, &QPushButton::clicked, [=]() {
        glWidget->togglePause();
    });

    /// Ajouter un véhicule
    connect(btnAddVeh, &QPushButton::clicked, [=]() {
        auto& noeuds = glWidget->graphe.getNoeuds();
        if (noeuds.empty()) {
            qDebug() << "Graphe non chargé : impossible d'ajouter un véhicule";
            return;
        }
        int newId = glWidget->getVoitures().size() + 1;
        double vitesse = 0.1333;
        Noeud* depart = noeuds[rand() % noeuds.size()];
        glWidget->ajouterVoiture(Voiture(newId, depart, vitesse));
        glWidget->setVoitureSuivie(&glWidget->getVoitures().back());
        statVehicles->setText(QString("Véhicules : %1").arg(glWidget->getVoitures().size()));
        glWidget->update();
    });

    /// Supprimer le véhicule suivi
    connect(btnDelVeh, &QPushButton::clicked, [=]() {
        glWidget->supprimerVoitureSuivie();
        statVehicles->setText(QString("Véhicules : %1").arg(glWidget->getVoitures().size()));
        glWidget->update();
    });

    /// Charger un fichier OSM local
    connect(btnLoadMap, &QPushButton::clicked, this, [=]() {
        glWidget->detruireSimulation();

        QString osmFile = QFileDialog::getOpenFileName(
            this,
            "Charger un fichier OSM",
            "",
            "Fichiers OSM (*.osm)"
            );
        if (osmFile.isEmpty()) return;

        bool ok = false;
        int nbVoit = QInputDialog::getInt(
            this,
            "Nombre de véhicules",
            "Combien de véhicules à générer ?",
            1000, 0, 1'000'000, 1, &ok
            );
        if (!ok) return;

        glWidget->setLoading(true);
        QApplication::processEvents();

        glWidget->chargerGrapheEtVoitures(osmFile.toStdString(), nbVoit);

        glWidget->setLoading(false);

        statVehicles->setText(QString("Véhicules : %1").arg(glWidget->getVoitures().size()));
        statEdges->setText(QString("Arêtes : %1").arg(glWidget->graphe.getAretes().size()));

        glWidget->update();
    });

    /// Chargement OSM via Overpass API avec coordonnées
    netManager = new QNetworkAccessManager(this);

    connect(btnLoadMapCoord, &QPushButton::clicked, this, [=]() {
        // Demande des coordonnées
        bool ok1, ok2, ok3, ok4;
        double latS = QInputDialog::getDouble(this, "Latitude Sud", "Latitude Sud :", 47.730, -90, 90, 6, &ok1);
        if (!ok1) return;
        double lonW = QInputDialog::getDouble(this, "Longitude Ouest", "Longitude Ouest :", 7.280, -180, 180, 6, &ok2);
        if (!ok2) return;
        double latN = QInputDialog::getDouble(this, "Latitude Nord", "Latitude Nord :", 47.770, -90, 90, 6, &ok3);
        if (!ok3) return;
        double lonE = QInputDialog::getDouble(this, "Longitude Est", "Longitude Est :", 7.370, -180, 180, 6, &ok4);
        if (!ok4) return;

        bool okVeh = false;
        int nbVoit = QInputDialog::getInt(
            this,
            "Nombre de véhicules",
            "Combien de véhicules à générer ?",
            1000, 0, 1'000'000, 1, &okVeh
            );
        if (!okVeh) return;

        // Construction de la requête Overpass
        QString query = QString(
                            "[out:xml][timeout:180];"
                            "("
                            "   way[\"highway\"](%1,%2,%3,%4);"
                            "   >;"
                            ");"
                            "out body;"
                            ).arg(latS).arg(lonW).arg(latN).arg(lonE);

        QUrl url("https://lz4.overpass-api.de/api/interpreter");
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        glWidget->setLoading(true);
        glWidget->update();
        QApplication::processEvents();

        QNetworkReply* reply = netManager->post(req, "data=" + QUrl::toPercentEncoding(query));

        // Gestion du téléchargement
        connect(reply, &QNetworkReply::finished, this, [=]() {
            glWidget->setLoading(false);
            glWidget->update();

            if (reply->error() != QNetworkReply::NoError) {
                qDebug() << "Erreur Overpass :" << reply->errorString();
                reply->deleteLater();
                return;
            }

            QByteArray osmData = reply->readAll();

            if (!osmData.contains("<osm")) {
                qDebug() << "Le contenu téléchargé n'est pas un OSM valide !";
                reply->deleteLater();
                return;
            }

            QString filePath = "downloaded_map_coords.osm";
            QFile f(filePath);
            if (!f.open(QIODevice::WriteOnly)) {
                qDebug() << "Impossible d'écrire le fichier OSM.";
                reply->deleteLater();
                return;
            }
            f.write(osmData);
            f.close();

            try {
                glWidget->detruireSimulation();
                glWidget->chargerGrapheEtVoitures(filePath.toStdString(), nbVoit);
            } catch (...) {
                qDebug() << "Erreur chargement graphe.";
            }

            statVehicles->setText(QString("Véhicules : %1").arg(glWidget->getVoitures().size()));
            statEdges->setText(QString("Arêtes : %1").arg(glWidget->graphe.getAretes().size()));

            reply->deleteLater();
        });
    });
    connect(glWidget, &OpenGLWidget::simulationUpdated,
            this, &MainWindow::mettreAJourInterference);
}
/**
 * @brief Met à jour le label affichant le nombre d'arêtes d'interférence.
 * C'est le slot connecté au signal simulationUpdated() de OpenGLWidget.
 */
void MainWindow::mettreAJourInterference() {

    int nbInterferences = glWidget->getNombreAretesInterference();


    statDegree->setText(QString("Arêtes Interférence : %1").arg(nbInterferences));
}
