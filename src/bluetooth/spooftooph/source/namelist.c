/* 
   namelist.c
   Copyright (C) 2010 Shadow Cave LLC

   Written 2010 by JP Dunning (.ronin)
   ronin [ at ] shadowcave [dt] org
   <www.hackfromacave.com>
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation;

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
   IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY 
   CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES 
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
   OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

   ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS, 
   COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS SOFTWARE IS 
   DISCLAIMED.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char * rand_name(int i) {

  switch (i) {
	case 0: return "Jacob";
	case 1: return "Emma";
	case 2: return "Michael";
	case 3: return "Isabella";
	case 4: return "Ethan";
	case 5: return "Emily";
	case 6: return "Joshua";
	case 7: return "Madison";
	case 8: return "Daniel";
	case 9: return "Ava";
	case 10: return "Alexander";
	case 11: return "Olivia";
	case 12: return "Anthony";
	case 13: return "Sophia";
	case 14: return "William";
	case 15: return "Abigail";
	case 16: return "Christopher";
	case 17: return "Elizabeth";
	case 18: return "Matthew";
	case 19: return "Chloe";
	case 20: return "Jayden";
	case 21: return "Samantha";
	case 22: return "Andrew";
	case 23: return "Addison";
	case 24: return "Joseph";
	case 25: return "Natalie";
	case 26: return "David";
	case 27: return "Mia";
	case 28: return "Noah";
	case 29: return "Alexis";
	case 30: return "Aiden";
	case 31: return "Alyssa";
	case 32: return "James";
	case 33: return "Hannah";
	case 34: return "Ryan";
	case 35: return "Ashley";
	case 36: return "Logan";
	case 37: return "Ella";
	case 38: return "John";
	case 39: return "Sarah";
	case 40: return "Nathan";
	case 41: return "Grace";
	case 42: return "Elijah";
	case 43: return "Taylor";
	case 44: return "Christian";
	case 45: return "Brianna";
	case 46: return "Gabriel";
	case 47: return "Lily";
	case 48: return "Benjamin";
	case 49: return "Hailey";
	case 50: return "Jonathan";
	case 51: return "Anna";
	case 52: return "Tyler";
	case 53: return "Victoria";
	case 54: return "Samuel";
	case 55: return "Kayla";
	case 56: return "Nicholas";
	case 57: return "Lillian";
	case 58: return "Gavin";
	case 59: return "Lauren";
	case 60: return "Dylan";
	case 61: return "Kaylee";
	case 62: return "Jackson";
	case 63: return "Allison";
	case 64: return "Brandon";
	case 65: return "Savannah";
	case 66: return "Caleb";
	case 67: return "Nevaeh";
	case 68: return "Mason";
	case 69: return "Gabriella";
	case 70: return "Angel";
	case 71: return "Sofia";
	case 72: return "Isaac";
	case 73: return "Makayla";
	case 74: return "Evan";
	case 75: return "Avery";
	case 76: return "Jack";
	case 77: return "Riley";
	case 78: return "Kevin";
	case 79: return "Julia";
	case 80: return "Jose";
	case 81: return "Leah";
	case 82: return "Isaiah";
	case 83: return "Aubrey";
	case 84: return "Luke";
	case 85: return "Jasmine";
	case 86: return "Landon";
	case 87: return "Audrey";
	case 88: return "Justin";
	case 89: return "Katherine";
	case 90: return "Lucas";
	case 91: return "Morgan";
	case 92: return "Zachary";
	case 93: return "Brooklyn";
	case 94: return "Jordan";
	case 95: return "Destiny";
	case 96: return "Robert";
	case 97: return "Sydney";
	case 98: return "Aaron";
	case 99: return "Alexa";    
    }
}

static char * scifi_name(int i) {

  switch (i) {
        case 0: return "Spock";                                              
        case 1: return "Chekov";                                             
        case 2: return "Starbuck";                                           
        case 3: return "Jean-Luc Picard";                                    
        case 4: return "Quark";                                              
        case 5: return "Gandolf";                                            
        case 6: return "Frodo Baggins";                                      
        case 7: return "Hermione";                                           
        case 8: return "Dr. Who";                                            
        case 9: return "John Sheridan";                                      
        case 10: return "Delenn";                                            
        case 11: return "Samantha Carter";                                   
        case 12: return "John Criton";                                       
        case 13: return "Moya";                                              
        case 14: return "G'Kar";                                             
        case 15: return "Lee Adama";                                         
        case 16: return "Spot";                                              
        case 17: return "Lwaxana Troy";                                      
        case 18: return "Reginald Barclay";                                  
        case 19: return "Jadzia Dax";                                        
        case 20: return "Kes";                                               
        case 21: return "Zathras";                                           
        case 22: return "Porkins";                                           
        case 23: return "Alfred Bester";                                     
        case 24: return "Walter Harriman";                                   
        case 25: return "Selmak";                                            
        case 26: return "Gort";                                              
        case 27: return "Wedge";                                             
        case 28: return "Greedo";                                            
        case 29: return "Moya";                                              
        case 30: return "Stark";                                             
        case 31: return "Luna";                                              
        case 32: return "Malcolm Reynolds";                                  
        case 33: return "Jayne Cobb";                                        
        case 34: return "Inara Serra";                                       
        case 35: return "Boomer";                                            
        case 36: return "Gaius Baltar";                                      
        case 37: return "Saul Tigh";                                         
        case 38: return "Laura Roslin";                                      
        case 39: return "Lee Adama";                                         
        case 40: return "Londo Mollari";                                     
        case 41: return "Lennier";                                           
        case 42: return "Jaime Sommers";                                     
        case 43: return "Oscar Goldman";                                     
        case 44: return "Steve Austin";                                      
        case 45: return "Dr. Zachary Smith";                                 
        case 46: return "Will Robinson";                                     
        case 47: return "Penny Robinson";                                    
        case 48: return "Tom Servo";                                         
        case 49: return "Crow T. Robot";                                     
        case 50: return "Walter Skinner";                                    
        case 51: return "CGB Spender";                                       
        case 52: return "Dana Scully";                                       
        case 53: return "Fox Mulder";                                        
        case 54: return "Rick Deckard";                                      
        case 55: return "Sam Beckett";                                       
        case 56: return "Al Calavicci";                                      
        case 57: return "Eowyn";                                             
        case 58: return "Elrond";                                            
        case 59: return "Gandalf";                                           
        case 60: return "Denethor";                                          
        case 61: return "Olivia Dunham";                                     
        case 62: return "Walter Bishop";                                     
        case 63: return "Peter Bishop";
        case 64: return "Joh Fredersen";
        case 65: return "Neo";
        case 66: return "Morpheus";
        case 67: return "Akira";
        case 68: return "Riddick";
        case 69: return "Dr. Who";
        case 70: return "Philip J. Fry";
        case 71: return "Leela";
        case 72: return "Bender Bending Rodríguez";
        case 73: return "Zapp Brannigan";
        case 74: return "Hubert J. Farnsworth";
        case 75: return "Master / Blaster";
        case 76: return "Thorn";
        case 77: return "Snake Plissken";
        case 78: return "Clu";
        case 79: return "Tron";
        case 80: return "Sark";
        case 81: return "Sam Lowry";
        case 82: return "Korben Dallas";
        case 83: return "Zorg";
        case 84: return "Leeloo";
        case 85: return "Luke Skywalker";
        case 86: return "Han Solo";
        case 87: return "Princess Leia";
        case 88: return "Darth Vader";
        case 89: return "Obi-Wan Kenobi";
        case 90: return "R2-D2";
        case 91: return "Han Solo";
        case 92: return "Yoda";
        case 93: return "Chewbacca";
        case 94: return "Lando Calrissian";
        case 95: return "Saruman";
        case 96: return "C-3PO";
        case 97: return "Elrond";
        case 98: return "Gollum";
        case 99: return "Gimli";
  }
}
