<?php
$ast = Ast::parseString(<<<EOD
echo "Hello world\n";
function foo() {
  echo "Goodbye";
}
EOD
);

//echo $ast->export();
?>