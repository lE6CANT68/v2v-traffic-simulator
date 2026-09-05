# V2V Traffic Simulator

Un simulateur de trafic routier et de connexions réseaux "Vehicle-to-Vehicle" (V2V) haute performance, développé en C++. Ce projet permet de modéliser et de visualiser en temps réel les interactions de communication entre des milliers de véhicules sur un réseau routier réel.

## Démo

![Démo du simulateur V2V](./Gif_voiture.gif)

## Fonctionnalités Principales

* **Génération via OpenStreetMap (OSM) :** Chargement dynamique des routes à partir de fichiers locaux `.osm` ou via des requêtes API (Overpass) en utilisant des coordonnées GPS.
* **Rendu Haute Performance :** Affichage fluide de plusieurs milliers de véhicules en temps réel grâce à l'accélération matérielle **OpenGL** intégrée à l'interface **Qt**.
* **Algorithme d'Optimisation :** Implémentation d'une grille de partitionnement spatial (*Spatial Hashing*) réduisant drastiquement la complexité pour le calcul continu du graphe d'interférences V2V.
* **Interface Interactive :** Contrôle direct de la vitesse de simulation, ajustement du rayon d'interférence des antennes, navigation (zoom/drag) et suivi de véhicules ciblés.

## Documentation

La documentation complète du projet est disponible dans le dossier [`documentation`](./documentation) de ce dépôt.

## Prérequis et Compilation

Ce projet utilise le framework Qt et le système de build `qmake`. Pour le compiler, vous aurez besoin de :

1. **Qt Framework** (modules requis : Core, Gui, Widgets, Network, OpenGL).
2. **Qt Creator** (recommandé pour gérer le fichier projet).
3. Un compilateur C++ (MinGW, GCC, Clang ou MSVC).

**Installation :**

1. Clonez ce dépôt :

```bash
git clone https://github.com/lE6CANT68/v2v-traffic-simulator.git
```

2. Ouvrez le fichier `Projet_Reseau_Mobile.pro` dans votre IDE Qt Creator.
3. Configurez votre kit de compilation et lancez le build (Ctrl+R / Cmd+R).

## Crédits et Dépendances

* **Interface et Rendu** : Réalisés de zéro en C++ avec les frameworks [Qt](https://www.qt.io/) et OpenGL.
* **Parsing XML** : Le traitement des données cartographiques OSM est assuré par l'excellente bibliothèque open-source [tinyxml2](https://github.com/leethomason/tinyxml2), développée par Lee Thomason (les fichiers sources `tinyxml2.cpp` et `tinyxml2.h` sont inclus dans ce dépôt).
  Dépôt GitHub de la bibliothèque : https://github.com/leethomason/tinyxml2
