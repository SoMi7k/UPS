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

bool Player::checkPlayedCard(CardSuits trickSuit, std::optional<CardSuits> trumph, const Card& playedCard,
                             const std::map<int, Card>& playedCards, const Mode& mode) {
    std::vector<Card> validCards;
    for (const auto& c : playedCards) {
        validCards.push_back(c.second);
    }

    // 1. má hráč stejnou barvu?
    if (hand.findCardBySuit(trickSuit)) {
        if (playedCard.getSuit() != trickSuit) {
            invalid_move = "Musíš zahrát stejnou barvu!\n";
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
                    invalid_move = "Musíš zahrát vyšší kartu!\n";
                    return false;
                }
            }
        }
        return true;
    }


    // 2. nemá barvu, ale má trumf
    if (trumph.has_value()) {
        CardSuits suit = trumph.value();
        if (hand.findCardBySuit(suit)) {
            if (playedCard.getSuit() != trumph) {
                invalid_move = "Musíš zahrát trumf!\n";
                return false;
            }

            bool played_trumph = false;
            for (auto card :playedCards) {
                if (card.second.getSuit() == suit) {
                    played_trumph = true;
                }
            }

            if (played_trumph) {
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
                            invalid_move = "Musíš zahrát vyšší trumf!\n";
                            return false;
                        }
                    }
                }
            }
            return true;
        }
    }

    return true;
}

void Player::sortHand(const Mode& mode) {
    hand.sort(mode);
}

Hand& Player::getHand() {
    return hand;
}

int Player::getNumber() {
    return number;
}

std::string Player::getNick() {
    return nick;
}

std::string Player::getInvalidMove() {
    return invalid_move;
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