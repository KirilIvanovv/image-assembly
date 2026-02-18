import 'package:dart/template.dart';


void main() async 
{
  try 
  {
    Template myTemplate = await Template.load('test/assembly.image');
    myTemplate.printBytecode(0, 10, 10);
  } 
  catch (e) { print('Error: $e'); }
}
