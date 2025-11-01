#include <algorithm>
#include <utility>

#include "Player.hpp"
#include "Card.hpp"

Player::Player(int number, std::string nick) : number(number), nick(std::move(nick)) {}

void Player::removeHand() {
    hand.removeHand();
}

void Player::addCard(const Card& card) {
    hand.addCard(card);
}

void Player::addWonCard(const Card& card) {
    hand.addWonCard(card);
}

bool Player::hasCardInHand() const {
    return !hand.getCards().empty();
}

bool Player::hasWon() const {
    return !hand.getWonCards().empty();
}

Card Player::chooseCard() {
    std::string input;
    while (true) {
        std::cout << "Kterou kartu chcete zahrát?\n> ";
        std::cin >> input;
        try {
            Card playedCard = cardMapping(input);
            if (hand.findCardInHand(playedCard)) {
                std::cout << "Zahrál jste " << playedCard.toString() << std::endl;
                return playedCard;
            } else {
                std::cout << "Karta není ve tvém balíčku! Zkus to znovu.\n";
            }
        } catch (const std::invalid_argument&) {
            std::cout << "Zadal jsi špatný kód karty! Zkus to znovu.\n";
        }
    }
}

Card Player::play(CardSuits* trickSuit,
                  CardSuits trumph,
                  Card playedCard,
                  std::vector<Card*>& playedCards,
                  const Mode& mode) {
    hand.removeCard(playedCard);
    if (trickSuit) {
        while (!checkPlayedCard(*trickSuit, trumph, playedCard, playedCards, mode)) {
            hand.addCard(playedCard);
            playedCard = chooseCard();
            hand.removeCard(playedCard);
        }
    }
    return playedCard;
}

std::vector<Card> Player::pickCards(int count) {
    std::vector<Card> result;
    const auto& cards = hand.getCards();

    if (count == -1 || count > static_cast<int>(cards.size()))
        count = static_cast<int>(cards.size());

    result.insert(result.end(), cards.begin(), cards.begin() + count);
    return result;
}

int Player::calculateHand(const Mode& mode) {
    hand.calculateHand(mode);
    return hand.getPoints();
}

bool Player::checkPlayedCard(CardSuits trickSuit,
                             CardSuits trumph,
                             const Card& playedCard,
                             const std::vector<Card*>& playedCards,
                             const Mode& mode) {
    std::vector<Card> validCards;
    for (const auto& c : playedCards) {
        validCards.push_back(*c);
    }

    // 1. má hráč stejnou barvu?
    if (hand.findCardBySuit(trickSuit)) {
        if (playedCard.getSuit() != trickSuit) {
            std::cout << "Musíš zahrát stejnou barvu!\n";
            return false;
        }

        auto highestInSuit = std::max_element(validCards.begin(), validCards.end(),
            [&](const Card& a, const Card& b) {
                return a.getSuit() == trickSuit &&
                       a.getValue(mode) < b.getValue(mode);
            });

        if (highestInSuit != validCards.end() &&
            playedCard.getValue(mode) < highestInSuit->getValue(mode)) {
            for (const auto& c : hand.getCards()) {
                if (c.getSuit() == trickSuit &&
                    c.getValue(mode) > highestInSuit->getValue(mode)) {
                    std::cout << "Musíš zahrát vyšší kartu!\n";
                    return false;
                }
            }
        }
        return true;
    }

    // 2. nemá barvu, ale má trumf
    if (hand.findCardBySuit(trumph)) {
        if (playedCard.getSuit() != trumph) {
            std::cout << "Musíš zahrát trumf!\n";
            return false;
        }

        auto highestTrump = std::max_element(validCards.begin(), validCards.end(),
            [&](const Card& a, const Card& b) {
                return a.getSuit() == trumph &&
                       a.getValue(mode) < b.getValue(mode);
            });

        if (highestTrump != validCards.end() &&
            playedCard.getValue(mode) < highestTrump->getValue(mode)) {
            for (const auto& c : hand.getCards()) {
                if (c.getSuit() == trumph &&
                    c.getValue(mode) > highestTrump->getValue(mode)) {
                    std::cout << "Musíš zahrát vyšší trumf!\n";
                    return false;
                }
            }
        }
        return true;
    }

    // 3. nemá ani barvu ani trumf → může cokoli
    return true;
}

void Player::sortHand(const Mode& mode) {
    hand.sort(mode);
}

Hand Player::getHand() {
    return hand;
}

int Player::getNumber() {
    return number;
}

std::string Player::getNick() {
    return nick;
}

std::string Player::toString() const {
    std::string output = "Player #" + std::to_string(number) + ": ";
    const auto& cards = hand.getCards();
    for (size_t i = 0; i < cards.size(); ++i) {
        output += cards[i].toString();
        if (i + 1 < cards.size()) output += ", ";
    }
    return output;
}
