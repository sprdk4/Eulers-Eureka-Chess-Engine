// AI
// This is where you build your AI

#include <time.h>
#include <algorithm>
#include <bitset>
#include <sstream>
#include <fstream>
#include "ai.hpp"
#include "my_code/ScopedBenchmark.h"
#include "my_code/BitBoard.h"
#include "my_code/BitMath.h"
#include "my_code/MoveSelection.h"
#include "my_code/Pondering.h"
#include "my_code/OpeningTable.h"
// You can add #includes here for your AI.

BitBoard::BitBoard bitboard;
extern int64_t grandNodesExplored;
extern int64_t grandMovesGenerated;
std::stringstream toAddToBook;
BitBoard::OpeningTable openingTable;

namespace cpp_client {

    namespace chess {

/// <summary>
/// This returns your AI's name to the game server.
/// Replace the string name.
/// </summary>
/// <returns>The name of your AI.</returns>
        std::string AI::get_name() const {
            return "Eulers Eureka - Shawn Roach";
        }

/// <summary>
/// This is automatically called when the game first starts, once the game objects are created
/// </summary>
        void AI::start() {
            if (player->color == "Black") {
                std::cout << "I'm Black\n";
            } else {
                std::cout << "I'm White\n";
            }
            //print_current_board();
            bitboard = {game, player};
            bitboard.switchSides();
            bitboard.switchSides();
            BitBoard::createMagicLookupTable();

            openingTable = {bitboard.side};

            // This is a good place to initialize any variables
            auto seed = time(NULL);
            //seed = 1489019032; //used for 32 turn benchmark
            std::cout << "seed = " << seed << ";\n";
            srand(seed);
            //std::atexit(BitBoard::endPonderingThread); //is this necessary? I'm not familiar enough with multithreading
            BitBoard::startPonderingThread();
            BitBoard::init();
        }

/// <summary>
/// This is automatically called the game (or anything in it) updates
/// </summary>
        void AI::game_updated() {
            // If a function you call triggers an update this will be called before it returns.
        }

/// <summary>
/// This is automatically called when the game ends.
/// </summary>
/// <param name="won">true if you won, false otherwise</param>
/// <param name="reason">An explanation for why you either won or lost</param>
        void AI::ended(bool won, const std::string &reason) {
            BitBoard::endPonderingThread();
            std::cout << "game ending board state: \n";
            print_current_board();
            std::cout << "total turns: " << game->current_turn << '\n';

            std::cout << "grandNodesExplored:  " << grandNodesExplored << '\n';
            std::cout << "grandMovesGenerated: " << grandMovesGenerated << '\n';
            ScopedBenchmark::printBenchMarks();

            std::cout << "toAddToBook:\n";
            std::cout << ((bitboard.side) ? "white" : "black") << '\n';
            std::cout << "zobristHash move depth score expectedTimeForNextDepth\n";
            std::cout << toAddToBook.str();
            std::cout << std::flush;

            std::string tableFileToSaveTo = "arenaupload/";
            if (bitboard.side == BitBoard::white) {
                tableFileToSaveTo += "white";
            }else{
                tableFileToSaveTo += "black";
            }
            std::stringstream ss;
            ss << time(NULL);
            tableFileToSaveTo+=ss.str();
            tableFileToSaveTo+=".txt";
            std::cout<<"saved new opening table entries to: "<<tableFileToSaveTo<<'\n';

            std::ofstream newtable(tableFileToSaveTo, std::ios_base::app);
            newtable<<toAddToBook.str();
            newtable.close();
            // You can do any cleanup of your AI here.  The program ends when this function returns.
        }


/// <summary>
/// This is called every time it is this AI.player's turn.
/// </summary>
/// <returns>Represents if you want to end your turn. True means end your turn, False means to keep your turn going and re-call this function.</returns>
        bool AI::run_turn() {

            SCOPED_BENCHMARK("run_turn()");
            BitBoard::endPonderingThread();
            // Here is where you'll want to code your AI.

            // We've provided sample code that:
            //    1) prints the board to the console
            //    2) prints the opponent's last move to the console
            //    3) prints how much time remaining this AI has to calculate moves
            //    4) makes a random move.

            // 1) print the board to the console
            //print_current_board();

            // 2) print the opponent's last move to the console
            if (game->moves.size() > 0) {
                std::cout << "Opponent's Last Move: '" << game->moves[game->moves.size() - 1]->san << "'"
                          << "  on turn: " << game->current_turn << std::endl;
                bitboard.performMove(game->moves[game->moves.size() - 1]);
            }
            // 3) print how much time remaining this AI has to calculate moves
            std::cout << "Time Remaining: " << player->time_remaining << " ns" << std::endl;


            // 4) make a random move.
            std::cout << "fen: \"" << game->fen << "\"\n";
            std::cout << "zobirst hash: " << std::hex << bitboard.zobristHash << std::dec << '\n';

            bitboard.printBoard();
            auto selectedMove = BitBoard::getBestMove(player->time_remaining);
            bitboard.performMove(selectedMove);


            std::string fromFile = BitBoard::toFile(selectedMove.from());
            std::string toFile = BitBoard::toFile(selectedMove.to());
            int fromRank = BitBoard::toRank(selectedMove.from());
            int toRank = BitBoard::toRank(selectedMove.to());
            std::cout << "making move: " << fromFile << fromRank << " to " << toFile << toRank << '\n';
            std::string promotion;
            switch (selectedMove.promotionType()) {
                case BitBoard::queens:
                    promotion = "Queen";
                    break;
                case BitBoard::knights:
                    promotion = "Knight";
                    break;
                case BitBoard::rooks:
                    promotion = "Rook";
                    break;
                case BitBoard::bishops:
                    promotion = "Bishop";
                    break;
                default:
                    promotion = "";
                    break;
            }

            if (promotion != "") {
                std::cout << "\t with promotion: " << promotion << '\n';
            }

            BitBoard::startPonderingThread();

            for (auto &e : player->pieces) {
                if (e->rank == fromRank && e->file == fromFile) {
                    e->move(toFile, toRank, promotion);
                    return true;
                }
            }
            std::cout << "error: did not find piece to move\n";
            return true; // to signify we are done with our turn.
        }

/// <summary>
///  Prints the current board using pretty ASCII art
/// </summary>
/// <remarks>
/// Note: you can delete this function if you wish
/// </remarks>
        void AI::print_current_board() {
            for (int rank = 9; rank >= -1; rank--) {
                std::string str = "";
                if (rank == 9 || rank == 0) { // then the top or bottom of the board
                    str = "   +------------------------+";
                } else if (rank == -1) { // then show the ranks
                    str = "     a  b  c  d  e  f  g  h";
                } else {// board
                    str += " ";
                    str += std::to_string(rank);
                    str += " |";
                    // fill in all the files with pieces at the current rank
                    for (int file_offset = 0; file_offset < 8; file_offset++) {
                        std::string file(1, 'a' + file_offset); // start at a, with with file offset increasing the char;
                        chess::Piece current_piece = nullptr;
                        for (const auto &piece : game->pieces) {
                            if (piece->file == file && piece->rank == rank) { // then we found the piece at (file, rank)
                                current_piece = piece;
                                break;
                            }
                        }

                        char code = '.'; // default "no piece";
                        if (current_piece != nullptr) {
                            code = current_piece->type[0];

                            if (current_piece->type == "Knight") { // 'K' is for "King", we use 'N' for "Knights"
                                code = 'N';
                            }
                            if (current_piece->owner->id ==
                                "1") { // the second player (black) is lower case. Otherwise upppercase already
                                code = tolower(code);
                            }
                        }
                        str += " ";
                        str += code;
                        str += " ";
                    }
                    str += "|";
                }
                std::cout << str << std::endl;
            }
        }

// You can add additional methods here for your AI to call

    } // chess

} // cpp_client
