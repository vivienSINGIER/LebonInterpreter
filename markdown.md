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
`koz` commentaire sur une ligne `finkoz`
```

### 4.2. Variables

```text
basaz nom idon valeur
```

Exemple :

```text
keksoz age ifér 18
keksoz nom saidon "Alice"
```

### 4.3. Opérations

```text
keksoz total ifér 5 plis 3 * 2
```

Les opérations supportées peuvent inclure :

- addition : `+` = `ek`/`anplis`/`plis`/`azout`
- soustraction : `-` = `mwin`/`rotir`/`anlèv` 
- multiplication : `*` = `fwa`/`fwa fwa`/`miltipli`
- division : `/` = `koup`/`partaz`/`kasan`
- comparaison : `==` = `parey`/`égal`/`mem`, `!=` = `pa-égal`/`diferan`/`pa-parey`, `<` = `pli-piti`/`piti`/`anba`, `>` =  `dépas`/`gran`/`plis-gran`, `<=` = `pli-piti-egal`/`pa-gran`, `>=` = `pa-piti` / `plis-gran-egal`


### 4.4. Affichage

```text
afise "Bonjour"
afise age
```

## 5. Structures de contrôle

### 5.1. Condition

```text
kan condition 
ouver
    instruction1
    instruction2
laFin 
otreman 
ran
    instruction3
setou
```

Exemple :

```text
kan age plis-gran-egal 18 ouver afise "Majeur" ferm
lot lodebi afise "Mineur" setou
```

### 5.2. Boucle

```text
toultan condition 
ouver
    instruction
laFin
```

Exemple :

```text
keksoz i idon 0
tanki i pli-piti 5 
rant
    afise i
    bazar i poufer i plis 1
ferm
```

## 6. Fonctions

```text
fonksyon nom(param1, param2) 
ouver
    instruction
    ala valeur
ferm
```

Exemple :

```text
zafer addition(a, b) 
ouver
    ran a zout b
setou

keksoz result saidon addition(2, 3)
afise result
```

## 7. Types supportés

- flottant : `3.14`
- chaîne : `"lebon"`
- bool : `true` = `pa-fo`, `false` = `pa-vré`

## 8. Portée des variables

- Variables locales : visibles uniquement dans la fonction ou le bloc courant

## 9. Exemple complet

```text
keksoz nom = "Nathan"
bazar age = 20

kan age pa-pit 18 ouver afise "Bienvenue " ek nom ferm
otreman ouver afise "Accès refusé" setou

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

- mots-clés : `set` = `keksoz`/`bazar`, `if` = `kan`/`si-sa`/`si`, `else` = `sinon`/`lot`/`otreman`, `else if` = `sinon-si`/`si-ankor`/`lot-si`
        `while` = `tanki`/`toultan`, `func` = `zafer`/`fonksyon`/`travay`, `return` = `ran`/`rovoy`/`donn`/`ala`, `print` = `afise`, `for`= `pou`, `break` = `aret`/`stop`, `continue` = `kontinie`/`poursiv`
- littéraux : nombres, chaînes, booléens
- opérateurs : `+` = `ek`/`anplis`/`plis`/`azout`, `-` = `mwin`/`rotir`/`anlèv`, `*` = `fwa`/`fwa fwa`/`miltipli`, `/` = `koup`/`partaz`/`kasan`, `==` = `leparay`/`lomen`
- ponctuations : `(`, `)`, `{` = `ouver`/`rant`/`lodebi`, `}` = `ferm`/`setou`/`sorti`/`laFin`, `,`, `=` = `idon`/`poufer`/`ifér`/`le`/`saidon`


## 11. Prochaine évolution possible

- ajout d'un effet drunk
- ajout de plus de type de variable

## 12. Résumé

Ce langage fait TOURNER LA TÊTE !

---