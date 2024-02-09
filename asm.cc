#include <iostream>
#include <string>
#include <vector>
#include "scanner.h"
#include <unordered_map>
using namespace std;

bool valid_int(const Token& tk) {
  int64_t num = tk.toNumber();
  int res;
  if (tk.getKind() == Token::HEXINT) {
    res = ((num > 0xffffffff) || (num < 0x00000000)) ? 0 : 1;
  }
  if (tk.getKind() == Token::INT) {
    res = ((num > 4294967295 || num < -2147483648)) ? 0 : 1;
  }
  return res;
}

bool valid_immediate(const Token& imm, unordered_map<string, int> symbolTable, int& jr, int currentAddress) {
  if (imm.getKind() == Token::ID) {
    if (symbolTable.count(imm.getLexeme()) == 0) {
      return 0;
    }
    jr = (symbolTable[imm.getLexeme()] - currentAddress - 4) / 4;
    return 1;
  }

  jr = imm.toNumber();
  int res;
  if (imm.getKind() == Token::HEXINT) {
    res = ((jr > 0xffff) || (jr < 0x0000)) ? 0 : 1;
  }
  if (imm.getKind() == Token::INT) {
    res = ((jr > 32767 || jr < -32768)) ? 0 : 1;
  }
  return res;
}

void output(int64_t word) {
  for (int shift = 24; shift >= 0; shift -= 8) {
    unsigned char c = (word >> shift) & 0xff;
    std::cout << c;
  }
}

bool reg_in_range(Token target) {
  if (target.toNumber()<0 || target.toNumber()>31) return false;
  return true;
}

int main() {
  std::string line;
  unordered_map<string, int> symbolTable;
  vector<vector<Token>> lines;
  int currentAddress = 0;
  int64_t instr;
  bool if_label = false;
  try {
    while (getline(std::cin, line)) {
      std::vector<Token> tokenLine = scan(line);
      lines.emplace_back(tokenLine);
      bool otherID_exist = false;
      if (tokenLine.size() == 0) continue;
    
      for (auto &token : tokenLine) {
        if (token.getKind() == Token::LABEL) {
          string label = token.getLexeme();
          label.pop_back();
          if (symbolTable.count(label) != 0) throw(ScanningFailure("ERROR: Duplicate label"));
          symbolTable[label] = currentAddress;
        } else if (token.getKind() == Token::ID || 
        token.getKind() == Token::WORD) {
          otherID_exist = true;
        }
        if (otherID_exist) {
          currentAddress += 4;
          break;
        }
      }
    }

    currentAddress = 0;
    for (auto& tokenLine:lines) {
      int size = tokenLine.size();
      if (size == 0) continue;
      int i = 0; // keep track if there is a line starting in "label"
      if (tokenLine[0].getKind() == Token::LABEL) {
        while (i < size && (tokenLine[i].getKind() == Token::LABEL)) {
          if_label = true;
          i++;
        }
      }
      
      if (tokenLine[i].getKind() == Token::WORD) {
        if (i + 2 != size) throw(ScanningFailure("ERROR: there are too many tokens for (.word) ID"));

        if ((tokenLine[i+1].getKind() == Token::INT) ||
        (tokenLine[i+1].getKind() == Token::HEXINT)) {
          if (valid_int(tokenLine[i+1])) {
            int64_t num = tokenLine[i+1].toNumber();
            output(num);
          } else { throw(ScanningFailure("ERROR: input is out of bounds")); }
        } else if (tokenLine[i+1].getKind() == Token::ID) {
          if (symbolTable.count(tokenLine[i+1].getLexeme()) == 0) throw(ScanningFailure("ERROR: there does not exist the given label in the symbol Table"));
          
          string label = tokenLine[i+1].getLexeme();
          int64_t num = symbolTable[label];
          output(num);
        } else {
          throw(ScanningFailure("ERROR: there is no valid OP (.word)"));
        }

      } else if (tokenLine[i].getKind() == Token::ID) {
        // add slt sub sltu
        if (tokenLine[i].getLexeme() == "add" || tokenLine[i].getLexeme() == "slt" ||
        tokenLine[i].getLexeme() == "sub" || tokenLine[i].getLexeme() == "sltu") {
          if (tokenLine[i+1].getKind() != Token::REG || 
            tokenLine[i+2].getKind() != Token::COMMA || 
            tokenLine[i+3].getKind() != Token::REG || 
            tokenLine[i+4].getKind() != Token::COMMA || 
            tokenLine[i+5].getKind() != Token::REG || 
            i + 6 != size) {
            throw(ScanningFailure("ERROR: Add/sub/slt/sltu"));
          }
          Token target = tokenLine[i+1], s = tokenLine[i+3], t = tokenLine[i+5];
          if (!reg_in_range(target) || !reg_in_range(t) || !reg_in_range(s)) throw(ScanningFailure("ERROR: register out of bounds"));

          if (tokenLine[i].getLexeme() == "add") instr = (0<<26) | (s.toNumber()<<21) | (t.toNumber()<<16) | (target.toNumber()<<11) | 32;
          if (tokenLine[i].getLexeme() == "sub") instr = (0<<26) | (s.toNumber()<<21) | (t.toNumber()<<16) | (target.toNumber()<<11) | 34;
          if (tokenLine[i].getLexeme() == "slt") instr = (0<<26) | (s.toNumber()<<21) | (t.toNumber()<<16) | (target.toNumber()<<11) | 42;
          if (tokenLine[i].getLexeme() == "sltu") instr = (0<<26) | (s.toNumber()<<21) | (t.toNumber()<<16) | (target.toNumber()<<11) | 43;
          output(instr);
        // beq
        } else if (tokenLine[i].getLexeme() == "beq" || tokenLine[i].getLexeme() == "bne") {
          if (!(tokenLine[i+1].getKind() == Token::REG && 
                tokenLine[i+2].getKind() == Token::COMMA && 
                tokenLine[i+3].getKind() == Token::REG && 
                tokenLine[i+4].getKind() == Token::COMMA && 
                (tokenLine[i+5].getKind() == Token::INT || 
                tokenLine[i+5].getKind() == Token::HEXINT || 
                tokenLine[i+5].getKind() == Token::ID) && (i + 6 == size))) {
            throw(ScanningFailure("ERROR: there is too many tokens for the beq/bne operator"));
          }

          Token s = tokenLine[i+1], t = tokenLine[i+3], imm = tokenLine[i+5];
          int jr = 0;
          if (!reg_in_range(s) || !reg_in_range(t)) throw(ScanningFailure("ERROR: register is out of bounds"));
          
          if (!valid_immediate(imm, symbolTable, jr, currentAddress)) throw(ScanningFailure("ERROR"));

          if (tokenLine[i].getLexeme() == "beq") {
            instr = (4<<26) | (s.toNumber()<<21) | (t.toNumber()<<16) | (jr & 0xFFFF);
          } else {
            instr = (5<<26) | (s.toNumber()<<21) | (t.toNumber()<<16) | (jr & 0xFFFF);
          }
          output(instr);
        // mult. div
        } else if (tokenLine[i].getLexeme() == "mult" || tokenLine[i].getLexeme() == "div" || 
          tokenLine[i].getLexeme() == "multu" || tokenLine[i].getLexeme() == "divu") {

          if (tokenLine[i+1].getKind()!=Token::REG or tokenLine[i+2].getKind()!=Token::COMMA or 
              tokenLine[i+3].getKind()!=Token::REG or i + 4 != size){
            throw(ScanningFailure("ERROR: too many tokens (mult)"));
          }
          Token s = tokenLine[i+1], t = tokenLine[i+3];

          if (!reg_in_range(s) || !reg_in_range(t)) throw(ScanningFailure("ERROR: register is out of range"));

          if (tokenLine[i].getLexeme() == "div") instr = (0<<26) | (s.toNumber()<<21) | (t.toNumber()<<16) | 26;
          if (tokenLine[i].getLexeme() == "divu") instr = (0<<26) | (s.toNumber()<<21) | (t.toNumber()<<16) | 27;
          if (tokenLine[i].getLexeme() == "mult") instr = (0<<26) | (s.toNumber()<<21) | (t.toNumber()<<16) | 24;
          if (tokenLine[i].getLexeme() == "multu") instr = (0<<26) | (s.toNumber()<<21) | (t.toNumber()<<16) | 25;
          
          output(instr);
        
        } else if (tokenLine[i].getLexeme() == "jalr" ||
        tokenLine[i].getLexeme() == "jr") {
        Token s = tokenLine[i+1];
        if (s.getKind()!=Token::REG || i + 2 != size) throw(ScanningFailure("ERROR"));
        if (!reg_in_range(s)) throw(ScanningFailure("ERROR: register is out of range"));
        if (tokenLine[i].getLexeme() == "jalr") {
          instr = (0<<26) | (s.toNumber()<<21) | 9;
        } else {
          instr = (0<<26) | (s.toNumber()<<21) | 8;
        }
        output(instr);
        
        } else if (tokenLine[i].getLexeme() == "mfhi" || 
        tokenLine[i].getLexeme() == "mflo" || 
        tokenLine[i].getLexeme() == "lis") {
          Token d = tokenLine[i+1];
          if (d.getKind()!=Token::REG or i + 2 != size) throw(ScanningFailure("ERROR"));
          
          if (!reg_in_range(d)) throw(ScanningFailure("ERROR: register is out of range"));
  
          if (tokenLine[i].getLexeme() == "mfhi") {
            instr = (0<<16) | (d.toNumber()<<11) | 16;
          } else if (tokenLine[i].getLexeme() == "mflo") {
            instr = (0<<16) | (d.toNumber()<<11) | 18;
          } else {
            instr = (0<<16) | (d.toNumber()<<11) | 20;
          }
          output(instr);

        } else if (tokenLine[i].getLexeme() == "lw" || tokenLine[i].getLexeme() == "sw") {
          if (tokenLine[i+1].getKind()!=Token::REG || 
              tokenLine[i+2].getKind()!=Token::COMMA || 
              tokenLine[i+6].getKind()!=Token::RPAREN || 
              i + 7 != size || 
              tokenLine[i+4].getKind()!=Token::LPAREN || 
              tokenLine[i+5].getKind()!=Token::REG || 
              (tokenLine[i+3].getKind()!=Token::HEXINT && tokenLine[i+3].getKind()!=Token::INT)) {
            throw(ScanningFailure("ERROR"));
          }
          Token s = tokenLine[i+5], t = tokenLine[i+1], imm = tokenLine[i+3];

          if (!reg_in_range(s) || !reg_in_range(t)) throw(ScanningFailure("ERROR: register is out of range"));
          int target = imm.toNumber();
          if (imm.getKind() == Token::INT) {
            if (target > 32767 || target < -32768) throw(ScanningFailure("ERROR"));
          }
          if (imm.getKind() == Token::HEXINT) {
            if (target > 0xffff || target < 0) throw(ScanningFailure("ERROR"));
          }
          if (tokenLine[i].getLexeme() == "lw") {
            instr = (35 << 26) | (s.toNumber() << 21) | (t.toNumber() << 16) | (target & 0xffff);
          } else {
            instr = (43 << 26) | (s.toNumber() << 21) | (t.toNumber() << 16) | (target & 0xffff);
          }
          output(instr);

        } else if (if_label) {continue;} else { throw(ScanningFailure("ERROR: there is no valid identifier")); }
      } else if (if_label) { continue;
      } else {
        throw(ScanningFailure("ERROR: invalid"));
      }
      currentAddress += 4;
    }
      
  } catch (ScanningFailure &f) {
    std::cerr << f.what() << std::endl;
    return 1;
  }
  
  return 0;
}
