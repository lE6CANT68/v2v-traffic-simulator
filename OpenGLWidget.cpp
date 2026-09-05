#include "OpenGLWidget.h"
#include "qnetworkreply.h"
#include <iostream>
#include <ostream>


OpenGLWidget::OpenGLWidget(QWidget* parent)
    : QOpenGLWidget(parent),
    camX(0.0f),
    camY(0.0f),
    zoom(1.0f),
    dragging(false),
    frameCount(0),
    leftButtonPressed(false),
    clickTolerance(6),
    selectionRadiusPx(30),
    voitureSuivie(nullptr),
    zoomCible(1.0f),
    vitesseZoom(0.12f),
    followLissage(0.12f)
{
    netManager = new QNetworkAccessManager(this);

    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=]() {
        deplacerVoitures();
        update();
        emit simulationUpdated();
    });
    timer->start(16);

    // --- Slider pour le rayon ---
    QSlider* sliderRayon = new QSlider(Qt::Horizontal, this);
    sliderRayon->setRange(1, 200); // rayon possible entre 1 et 50 metres
    sliderRayon->setValue(int(Voiture::RAYON_INTERFERENCE));
    sliderRayon->setGeometry(10, 10, 150, 20);

    // --- Label pour afficher la valeur ---
    QLabel* labelRayon = new QLabel(this);
    labelRayon->setText(QString("Rayon : %1 m").arg(int(Voiture::RAYON_INTERFERENCE)));
    labelRayon->setGeometry(170, 10, 100, 20);
    labelRayon->setStyleSheet("color :white");

    // --- Connexion du slider ---
    connect(sliderRayon, &QSlider::valueChanged, this, [this, labelRayon](int value){
        Voiture::RAYON_INTERFERENCE = double(value);
        grapheInterference.setCaseSize( 2 * Voiture::RAYON_INTERFERENCE);
        //calculerAretesInterferenceVisibles();
        grapheInterference.calculer(voitures,camX,camY,width(),height(),zoom);
        labelRayon->setText(QString("Rayon : %1 m").arg(value));
        labelRayon->setStyleSheet("color: white; background-color: rgba(0,0,0,120); padding:2px;");
    });

    speedSlider = new QSlider(Qt::Horizontal, this);
    speedSlider->setRange(10, 5000);
    speedSlider->setValue(int(facteurVitesse * 100));
    speedSlider->setGeometry(10, 40, 150, 20);

    speedLabel = new QLabel(this);
    speedLabel->setGeometry(170, 40, 50, 20);
    speedLabel->setText(QString::number(speedSlider->value()) + "%");
    speedLabel->setStyleSheet("color: white; background-color: rgba(0,0,0,120); padding:2px;");

    connect(speedSlider, &QSlider::valueChanged, this, [this](int value){
        facteurVitesse = value / 100.0;
        speedLabel->setText(QString::number(value) + "%");
    });
}

void OpenGLWidget::setLoading(bool l) {
    loading = l;
    update();
}

std::vector<Voiture>& OpenGLWidget::getVoitures() {
    return voitures;
}

void OpenGLWidget::ajouterVoiture(const Voiture& v) {
    voitures.push_back(v);
}

void OpenGLWidget::setFacteurVitesse(double nb) {
    facteurVitesse = nb;
}

void OpenGLWidget::setVoitureSuivie(Voiture* v) {
    voitureSuivie = v;
    zoomCible = 2.5f;
}

void OpenGLWidget::supprimerVoiture(Voiture* v) {
    if (!v) return;


    int idx = -1;
    for (size_t i = 0; i < voitures.size(); ++i) {
        if (&voitures[i] == v) { idx = int(i); break; }
    }
    if (idx == -1) return;

    if (voitureSuivie == v) {
        voitureSuivie = nullptr;
        zoomCible = 1.0f;
        cacherInfoBulle();
    }

    voitures.erase(voitures.begin() + idx);

    grapheInterference.calculer(voitures,camX,camY,width(),height(),zoom);
}

void OpenGLWidget::supprimerVoitureSuivie() {
    if (!voitureSuivie) return;
    supprimerVoiture(voitureSuivie);
}

void OpenGLWidget::deplacerVoitures()
{
    if (frameCount % 5 == 0)
        grapheInterference.calculer(voitures, camX, camY, width(), height(), zoom);

    std::vector<std::vector<Voiture*>> proches(voitures.size());
    const auto& aretesInterference = grapheInterference.getAretes();
    for (auto& arete : aretesInterference) {
        int i = arete.first;
        int j = arete.second;
        proches[i].push_back(&voitures[j]);
        proches[j].push_back(&voitures[i]);
    }

    for (size_t i = 0; i < voitures.size(); ++i)
        voitures[i].deplacer(facteurVitesse, proches[i]);

    frameCount++;

    if (voitureSuivie) {
        zoom += (zoomCible - zoom) * vitesseZoom;
        if (zoom < 0.1f) zoom = 0.1f;

        float cibleX = voitureSuivie->getX() - width() / (2.0f * zoom);
        float cibleY = voitureSuivie->getY() - height() / (2.0f * zoom);

        camX += (cibleX - camX) * followLissage;
        camY += (cibleY - camY) * followLissage;
    }
    mettreAJourInfoBulle();
}

void OpenGLWidget::togglePause() {
    simulationPaused = !simulationPaused;

    if (simulationPaused) {
        lastSpeedValue = speedSlider->value();
        setFacteurVitesse(0.0);
        speedSlider->setEnabled(false);
        speedLabel->setText("0%");
    } else {
        setFacteurVitesse(lastSpeedValue / 100.0f);
        speedSlider->setEnabled(true);
        speedLabel->setText(QString::number(lastSpeedValue) + "%");
    }
}

void OpenGLWidget::chargerGrapheEtVoitures(const std::string& pathOSM, int nbVoitures)
{
    if (!graphe.chargerDepuisOSM(pathOSM)) {
        std::cerr << "Impossible de charger map.osm!" << std::endl;
        return;
    }
    minLat = 1e9;
    minLon = 1e9;
    maxLat = -1e9;
    maxLon = -1e9;
    for (auto n : graphe.getNoeuds()) {
        minLat = std::min(minLat, n->getLat());
        maxLat = std::max(maxLat, n->getLat());
        minLon = std::min(minLon, n->getLon());
        maxLon = std::max(maxLon, n->getLon());
    }

    double largeurGraph = maxLon - minLon;
    double hauteurGraph = maxLat - minLat;
    scale = 5000.0 / std::max(largeurGraph, hauteurGraph);

    for (auto n : graphe.getNoeuds()) {
        n->setX((n->getLon() - minLon) * scale);
        n->setY((n->getLat() - minLat) * scale);
    }
    double margeLat = 0.001;
    double margeLon = 0.001;

    double minLatG = 1e9, maxLatG = -1e9;
    double minLonG = 1e9, maxLonG = -1e9;
    for (auto n : graphe.getNoeuds()) {
        minLatG = std::min(minLatG, n->getLat());
        maxLatG = std::max(maxLatG, n->getLat());
        minLonG = std::min(minLonG, n->getLon());
        maxLonG = std::max(maxLonG, n->getLon());
    }

    minLatG -= margeLat; maxLatG += margeLat;
    minLonG -= margeLon; maxLonG += margeLon;

    auto [txMin, tyMax] = latLonToTile(minLatG, minLonG, zoomOSM);
    auto [txMax, tyMin] = latLonToTile(maxLatG, maxLonG, zoomOSM);

    for(int tx = txMin; tx <= txMax; ++tx)
        for(int ty = tyMin; ty <= tyMax; ++ty)
            chargerTuile(tx, ty, zoomOSM);

    auto& noeuds = graphe.getNoeuds();
    for (int i = 0; i < nbVoitures; ++i) {
        Noeud* depart = noeuds[rand() % noeuds.size()];
        double vitesse = 0.133;
        ajouterVoiture(Voiture(i + 1, depart, vitesse));
    }
}

void OpenGLWidget::detruireSimulation()
{
    voitures.clear();
    voitureSuivie = nullptr;

    if(infoBulle) {
        infoBulle->hide();
        infoBulle = nullptr;
    }

    graphe.clear();

    grapheInterference.clear();

    camX = camY = 0.0f;
    zoom = zoomCible = 1.0f;

    update();
}

void OpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
}

void OpenGLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, 0, h, -1, 1);
    glMatrixMode(GL_MODELVIEW);
}

void OpenGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    if (loading) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // --- Fond gris semi-transparent ---
        QColor bgColor(50, 50, 50, 180);
        painter.fillRect(rect(), bgColor);

        // --- Texte blanc ---
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 24, QFont::Bold));

        QRect rect = this->rect();
        painter.drawText(rect, Qt::AlignCenter, loadingText);

        painter.end();
        return;
    }

    // --- Transformation caméra / zoom ---
    glScalef(zoom, zoom, 1.0f);
    glTranslatef(-camX, -camY, 0.0f);

    float xMin = camX;
    float xMax = camX + width() / zoom;
    float yMin = camY;
    float yMax = camY + height() / zoom;

    // --- Calcul des bornes du graphe ---
    double minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9;
    for (auto n : graphe.getNoeuds()) {
        minX = std::min(minX, n->getX());
        maxX = std::max(maxX, n->getX());
        minY = std::min(minY, n->getY());
        maxY = std::max(maxY, n->getY());
    }

    // --- Dessin des tuiles OSM ---
    glEnable(GL_TEXTURE_2D);
    for (auto it = tuilesCache.begin(); it != tuilesCache.end(); ++it) {
        TuileOSM& tuile = it.value();
        if (tuile.image.isNull()) continue;

        if (tuile.textureId == 0) {
            glGenTextures(1, &tuile.textureId);
            glBindTexture(GL_TEXTURE_2D, tuile.textureId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            QImage img = tuile.image.mirrored(false, true).convertToFormat(QImage::Format_RGBA8888_Premultiplied);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(),
                         0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
        } else {
            glBindTexture(GL_TEXTURE_2D, tuile.textureId);
        }

        // --- Conversion tuile -> lat/lon -> X/Y ---
        auto tileXtoLon = [](int x, int z) {
            return x / double(1 << z) * 360.0 - 180.0;
        };
        auto tileYtoLat = [](int y, int z) {
            double n = M_PI - 2.0 * M_PI * y / double(1 << z);
            return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
        };

        double lon0 = tileXtoLon(tuile.x, tuile.z);
        double lon1 = tileXtoLon(tuile.x + 1, tuile.z);
        double lat0 = tileYtoLat(tuile.y + 1, tuile.z);
        double lat1 = tileYtoLat(tuile.y, tuile.z);

        float x0 = (lon0 - minLon) * scale;
        float x1 = (lon1 - minLon) * scale;
        float y0 = (lat0 - minLat) * scale;
        float y1 = (lat1 - minLat) * scale;

        if (x1 < xMin || x0 > xMax || y1 < yMin || y0 > yMax) continue;

        glBindTexture(GL_TEXTURE_2D, tuile.textureId);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex2f(x0, y0);
        glTexCoord2f(1,0); glVertex2f(x1, y0);
        glTexCoord2f(1,1); glVertex2f(x1, y1);
        glTexCoord2f(0,1); glVertex2f(x0, y1);
        glEnd();
    }
    glDisable(GL_TEXTURE_2D);

    // --- Graphe ---
    glLineWidth(2.0f);
    glColor3f(0.7f, 0.7f, 0.7f);
    glBegin(GL_LINES);
    for (auto& e : graphe.getAretes()) {
        float x1 = e.first->getX(), y1 = e.first->getY();
        float x2 = e.second->getX(), y2 = e.second->getY();
        if ((x1 >= xMin && x1 <= xMax && y1 >= yMin && y1 <= yMax) ||
            (x2 >= xMin && x2 <= xMax && y2 >= yMin && y2 <= yMax)) {
            glVertex2f(x1, y1);
            glVertex2f(x2, y2);
        }
    }
    glEnd();

    // --- Arêtes d'interférence ---
    glLineWidth(2.0f);
    glColor3f(1, 0, 0);
    glBegin(GL_LINES);
    for (auto [i, j] : grapheInterference.getAretes()) {
        glVertex2f(voitures[i].getX(), voitures[i].getY());
        glVertex2f(voitures[j].getX(), voitures[j].getY());
    }
    glEnd();
    glLineWidth(1.0f);

    // --- Voitures ---
    for (auto& v : voitures) {
        float vx = v.getX(), vy = v.getY();
        if (vx + Voiture::LONGUEUR_VOITURE / 2 < xMin || vx - Voiture::LONGUEUR_VOITURE / 2 > xMax ||
            vy + Voiture::LARGEUR_VOITURE / 2 < yMin || vy - Voiture::LARGEUR_VOITURE / 2 > yMax)
            continue;

        if (&v == voitureSuivie)
            drawCircle(vx, vy, std::max(Voiture::LONGUEUR_VOITURE, Voiture::LARGEUR_VOITURE), 32, QColor(255, 0, 255, 150));

        glPushMatrix();
        glTranslatef(vx, vy, 0);
        QColor c = v.getCouleur();
        glColor3f(c.redF(), c.greenF(), c.blueF());
        glBegin(GL_QUADS);
        glVertex2f(-Voiture::LONGUEUR_VOITURE / 2, -Voiture::LARGEUR_VOITURE / 2);
        glVertex2f(Voiture::LONGUEUR_VOITURE / 2, -Voiture::LARGEUR_VOITURE / 2);
        glVertex2f(Voiture::LONGUEUR_VOITURE / 2, Voiture::LARGEUR_VOITURE / 2);
        glVertex2f(-Voiture::LONGUEUR_VOITURE / 2, Voiture::LARGEUR_VOITURE / 2);
        glEnd();
        glPopMatrix();
    }
}

void OpenGLWidget::keyPressEvent(QKeyEvent* event)
{
    const float step = 50.0f;
    switch(event->key())
    {
    case Qt::Key_Left:  camX -= step; break;
    case Qt::Key_Right: camX += step; break;
    case Qt::Key_Up:    camY += step; break;
    case Qt::Key_Down:  camY -= step; break;
    case Qt::Key_Plus:  zoom *= 1.1f; break;
    case Qt::Key_Minus: zoom /= 1.1f; break;
    case Qt::Key_Escape: voitureSuivie = nullptr; zoomCible = 1.0f; break;
    }
    update();
}

void OpenGLWidget::mousePressEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton)
    {
        leftButtonPressed = true;
        mousePressPos = event->pos();
        lastMousePos = event->pos();
        dragging = false;
    }
    else if(event->button() == Qt::MiddleButton)
    {
        dragging = true;
        lastMousePos = event->pos();
    }
}

void OpenGLWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton && !dragging)
    {
        // --- Sélection de voiture ---
        float worldX = camX + event->pos().x() / zoom;
        float worldY = camY + (height() - event->pos().y()) / zoom;

        Voiture* selection = nullptr;
        float bestDist = 1e9f;

        for (auto& v : voitures)
        {
            float dx = v.getX() - worldX;
            float dy = v.getY() - worldY;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist < selectionRadiusPx / zoom && dist < bestDist)
            {
                selection = &v;
                bestDist = dist;
            }
        }

        if (selection)
        {
            voitureSuivie = selection;
            zoomCible = 2.5f;
            afficherInfoBulle(selection);
        }
        else
        {
            voitureSuivie = nullptr;
            zoomCible = 1.0f;
            cacherInfoBulle();
        }

        leftButtonPressed = false;
        dragging = false;
        update();
    }
    else if(event->button() == Qt::MiddleButton)
    {
        dragging = false;
    }
}

void OpenGLWidget::mouseMoveEvent(QMouseEvent* event)
{
    QPoint current = event->pos();
    if(leftButtonPressed && (current - mousePressPos).manhattanLength() > clickTolerance)
        dragging = true;

    if(dragging)
    {
        QPoint d = current - lastMousePos;
        camX -= d.x() / zoom;
        camY += d.y() / zoom;
        lastMousePos = current;
        update();
    }
}

void OpenGLWidget::wheelEvent(QWheelEvent* event)
{
    float factor = (event->angleDelta().y() > 0 ? 1.1f : 0.9f);

    QPointF mousePos = event->position();
    float mx = mousePos.x();
    float my = mousePos.y();

    float worldBeforeX = camX + mx / zoom;
    float worldBeforeY = camY + (height() - my) / zoom;

    zoom *= factor;
    if(zoom < 0.1f) zoom = 0.1f;
    if(zoom > 10.0f) zoom = 10.0f;

    float worldAfterX = camX + mx / zoom;
    float worldAfterY = camY + (height() - my) / zoom;
    camX += worldBeforeX - worldAfterX;
    camY += worldBeforeY - worldAfterY;

    update();
}

std::pair<int,int> OpenGLWidget::latLonToTile(double lat, double lon, int zoom) {
    int n = 1 << zoom;
    int x = int(n * ((lon + 180.0) / 360.0));
    int y = int(n * (1 - (log(tan(lat*M_PI/180.0) + 1/cos(lat*M_PI/180.0)) / M_PI)) / 2);
    return {x, y};
}

void OpenGLWidget::chargerTuile(int x, int y, int z) {
    if (tuilesCache.contains({x,y,z}) && !tuilesCache[{x,y,z}].image.isNull())
        return;

    QString url = QString("https://tile.openstreetmap.org/%1/%2/%3.png").arg(z).arg(x).arg(y);
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", "ProjetReseauMobile/1.0 (lorris.sanna@uha.fr)");
    QNetworkReply* reply = netManager->get(req);

    connect(reply, &QNetworkReply::finished, [=]() {
        QByteArray data = reply->readAll();
        QImage img;
        if (img.loadFromData(data)) {
            tuilesCache[{x,y,z}] = {x, y, z, img, 0};
            update();
        }
        reply->deleteLater();
    });
}

void OpenGLWidget::drawCircle(float cx, float cy, float radius, int segments, QColor color)
{
    glColor4f(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for(int i = 0; i <= segments; ++i)
    {
        float a = i * 2.0f * M_PI / segments;
        glVertex2f(cx + std::cos(a) * radius, cy + std::sin(a) * radius);
    }
    glEnd();
}



void OpenGLWidget::afficherInfoBulle(Voiture* v) {
    if (!v) return;

    // Cacher toutes les autres infobulles
    for (auto& voiture : voitures) {
        if (&voiture != v) voiture.cacherInfoBulle();
    }

    // Afficher celle de la voiture sélectionnée
    infoBulle = v->getInfoBulle(this);

    // --- Changer la couleur du texte en noir ---
    infoBulle->setStyleSheet("color: black; background-color: rgba(255,255,255,220); padding:2px;");

    float screenX = (v->getX() - camX) * zoom;
    float screenY = height() - (v->getY() - camY) * zoom; // Qt a y inversé

    // Décalage par rapport à la voiture
    int decalageX = 10;
    int decalageY = -50;

    infoBulle->move(int(screenX + decalageX), int(screenY + decalageY));
    infoBulle->raise();
    infoBulle->show();
}

void OpenGLWidget::cacherInfoBulle() {
    if (infoBulle) {
        infoBulle->hide();
    }
}



void OpenGLWidget::mettreAJourInfoBulle() {

    if (!voitureSuivie || !infoBulle) {
        return;
    }


    infoBulle->setText(QString("Voiture %1\nVitesse: %2 km/h")
                           .arg(voitureSuivie->getId())
                           .arg(int(voitureSuivie->getVitesseKmH())));

    float screenX = (voitureSuivie->getX() - camX) * zoom;
    float screenY = height() - (voitureSuivie->getY() - camY) * zoom;


    int decalageX = 10;
    int decalageY = -50;

    infoBulle->move(int(screenX + decalageX), int(screenY + decalageY));
}
