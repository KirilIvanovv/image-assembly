class OpCode {
  static const int noop = 0;
  static const int move = 2;
  static const int ret = 3;
  static const int constant = 4;
  static const int sample = 5;
  static const int add = 8;
  static const int multiply = 9;

  static int getLength(int operation) 
  {
    switch (operation) 
    {
      case noop:
      case ret:
        return 0;
      case move:
      case add:
      case multiply:
        return 1;
      case constant:
      case sample:
        return 5;
      default:
        return -1;
    }
  }

  static String getName(int operation) 
  {
    switch (operation) 
    {
      case noop: return "NOOP";
      case move: return "MOVE";
      case ret: return "RET ";
      case constant: return "LOAD";
      case sample: return "SMPL";
      case add: return "ADD ";
      case multiply: return "MULT";
      default: return "INVD";
    }
  }
}