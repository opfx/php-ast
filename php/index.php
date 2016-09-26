



<?php
$ast = Ast::parseString(<<<EOD
echo "Hello world\n";
function foo() {
  echo "Goodbye";
}
EOD
);

//echo $ast->export();
$astNode = Ast::getNode("O:\\work\\opfx\\repo\\.git\\php\\ext\\php-ast\\php\\index.php");
echo $astNode->export();
?>