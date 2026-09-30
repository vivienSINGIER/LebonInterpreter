# Recapitulatif du langage fait à la main

## 1. Objectif

Ce document permet de conserver une vue d'ensemble claire sur la logique, la grammaire et le comportement du langage.

## 2. Nom du langage

- Nom : `Lebon`
- Type : langage amusant et créole
- But : langage à faire retourner la tête

## 3. Philosophie

- Simplicity avant complexité
- Syntaxe lisible et proche du langage naturel
- Évaluation simple à interpréter
- Gestion explicite des variables, fonctions et structures de contrôle

## 4. Syntaxe générale

### 4.1. Commentaires

```text
``koz` commentaire sur une ligne `finkoz`
```

### 4.2. Variables

```text
set nom = valeur
```

Exemple :

```text
set age = 18
set nom = "Alice"
```

### 4.3. Opérations

```text
set total = 5 + 3 * 2
```

Les opérations supportées peuvent inclure :

- addition : `+`
- soustraction : `-`
- multiplication : `*`
- division : `/`
- comparaison : `==`, `!=`, `<`, `>`, `<=`, `>=`

### 4.4. Affichage

```text
print "Bonjour"
print age
```

## 5. Structures de contrôle

### 5.1. Condition

```text
if condition {
    instruction1
    instruction2
} else {
    instruction3
}
```

Exemple :

```text
if age >= 18 {
    print "Majeur"
} else {
    print "Mineur"
}
```

### 5.2. Boucle

```text
while condition {
    instruction
}
```

Exemple :

```text
set i = 0
while i < 5 {
    print i
    set i = i + 1
}
```

## 6. Fonctions

```text
func nom(param1, param2) {
    instruction
    return valeur
}
```

Exemple :

```text
func addition(a, b) {
    return a + b
}

set result = addition(2, 3)
print result
```

## 7. Types supportés

- flottant : `3.14`
- chaîne : `"lebon"`

## 8. Sémantique

### 8.1. Règles d'évaluation

- Les expressions sont évaluées de gauche à droite selon la priorité des opérateurs.
- Les variables sont stockées dans un environnement global ou local.
- Les blocs sont exécutés séquentiellement.
- Une fonction reçoit des arguments et renvoie une valeur avec `return` = `ran`/`rovoy`/`donn`/`ala`.

### 8.2. Portée des variables

- Variables globales : visibles partout
- Variables locales : visibles uniquement dans la fonction ou le bloc courant

## 9. Exemple complet

```text
keksoz nom = "Nathan"
bazar age = 20

if age >= 18 ouver afise "Bienvenue " ek nom ferm
else ouver afise "Accès refusé" setou

zafer carre(x) ouver rovoy x fwa x (ou x fwa fwa 2) laFin

afise carre(5)
```

Sortie attendue :

```text
Bienvenue Alice
25
```

## 10. Analyse du parseur

### 10.1. Étapes

1. Lecture du code source
2. Tokenisation
3. Parsing
4. Construction de l'AST
5. Exécution / interprétation

### 10.2. Tokens principaux

- mots-clés : `set` = `keksoz`/`bazar`, `if`, `else`, `while`, `func` = `zafer`/`fonksyon`/`travay`, `return` = `ran`/`rovoy`/`donn`/`ala`, `print` = `afise`
- identifiants : `nom`, `age`, `result`
- littéraux : nombres, chaînes, booléens
- opérateurs : `+` = `ek`/`anplis`/`plis`/`azout`, `-` = `mwin`/`rotir`/`anlèv`, `*` = `fwa`/`fwa fwa`/`miltipli`, `/` = `koup`/`partaz`/`kasan`, `==` = `leparay`/`lomen`
- ponctuations : `(`, `)`, `{` = `ouver`/`rant`/`lodebi`, `}` = `ferm`/`setou`/`sorti`/`laFin`, `,`, `=` = `idon`/`poufer`/`ifér`/`le`/`saidon`

## 11. Points d'attention

- gérer les erreurs de syntaxe proprement
- vérifier les types avant les opérations
- définir les règles de priorité des opérateurs
- gérer les blocs avec des `ouver` et `ferm`
- traiter les cas de variables non déclarées

## 12. Prochaine évolution possible

- ajout d'un effet drunk
- ajout de plus de type de variable

## 13. Résumé

Ce langage est une base simple et pédagogique pour comprendre la construction d'un langage fait main : analyse lexicale, syntaxique, gestion de variables, structures de contrôle et exécution. Il suffit de construire les briques de base pour étendre progressivement la puissance du langage.

---