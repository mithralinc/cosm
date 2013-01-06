<?php
/**
 * Database.php
 *
 * The Database class is meant to simplify the task of accessing
 * information from the website's database.
 *
 * Written by: Jpmaster77 a.k.a. The Grandmaster of C++ (GMC)
 * Last Updated: August 17, 2004
 */
include("constants.php");
include("cosm.php");

class MyPostgreSQL
{
   var $connection;         //The MySQL database connection

   /* Class constructor */
   function MyPostgreSQL()
   {
      /* Make connection to database */
      global $cssdk2_db_host, $cssdk2_db_name, $cssdk2_db_user, $cssdk2_db_pass;
      $connect_cmd = "host=" . $cssdk2_db_host . " dbname=" . $cssdk2_db_name .
        " user=" . $cssdk2_db_user . " password=" . $cssdk2_db_pass;
      $this->connection = pg_connect( $connect_cmd )
        or die( 'Could not connect: ' . pg_last_error() );
   }

   /**
    * confirmUserPass - Checks whether or not the given
    * username is in the database, if so it checks if the
    * given password is the same password in the database
    * for that user. If the user doesn't exist or if the
    * passwords don't match up, it returns an error code
    * (1 or 2). On success it returns 0.
    */
   function confirmUserPass($username, $password){
      /* Add slashes if necessary (for query) */
      if(!get_magic_quotes_gpc()) {
	      $username = addslashes($username);
      }

      /* Verify that user is in database */
      $q = "SELECT password FROM users WHERE username = '$username'";
      $result = pg_query( $this->connection, $q );
      if(!$result || (pg_numrows($result) < 1)){
         return 1; //Indicates username failure
      }

      /* Retrieve password from result, strip slashes */
      $dbarray = pg_fetch_array($result);
      $dbarray['password'] = stripslashes($dbarray['password']);
      $password = stripslashes($password);

      /* Validate that password is correct */
      if($password == $dbarray['password']){
         return 0; //Success! Username and password confirmed
      }
      else{
         return 2; //Indicates password failure
      }
   }

   /**
    * confirmUserID - Checks whether or not the given
    * username is in the database, if so it checks if the
    * given php_cookie is the same php_cookie in the database
    * for that user. If the user doesn't exist or if the
    * userids don't match up, it returns an error code
    * (1 or 2). On success it returns 0.
    */
   function confirmUserID($username, $php_cookie){
      /* Add slashes if necessary (for query) */
      if(!get_magic_quotes_gpc()) {
	      $username = addslashes($username);
      }

      /* Verify that user is in database */
      $q = "SELECT php_cookie FROM users WHERE username = '$username'";
      $result = pg_query($this->connection, $q);
      if(!$result || (pg_numrows($result) < 1)){
         return 1; //Indicates username failure
      }

      /* Retrieve php_cookie from result, strip slashes */
      $dbarray = pg_fetch_array($result);
      $dbarray['php_cookie'] = stripslashes($dbarray['php_cookie']);
      $php_cookie = stripslashes($php_cookie);

      /* Validate that php_cookie is correct */
      if($php_cookie == $dbarray['php_cookie']){
         return 0; //Success! Username and php_cookie confirmed
      }
      else{
         return 2; //Indicates php_cookie invalid
      }
   }

   /**
    * usernameTaken - Returns true if the username has
    * been taken by another user, false otherwise.
    */
   function usernameTaken($username){
      if(!get_magic_quotes_gpc()){
         $username = addslashes($username);
      }
      $q = "SELECT username FROM users WHERE username = '$username'";
      $result = pg_query($this->connection, $q);
      return (pg_numrows($result) > 0);
   }

   /**
    * usernameBanned - Returns true if the username has
    * been banned by the administrator.
    */
   function usernameBanned($username){
      if(!get_magic_quotes_gpc()){
         $username = addslashes($username);
      }
      $q = "SELECT username FROM banned_users WHERE username = '$username'";
      $result = pg_query($this->connection, $q);
      return (pg_numrows($result) > 0);
   }

   /**
    * addNewUser - Inserts the given (username, password, email)
    * info into the database. Appropriate user level is set.
    * Returns true on success, false otherwise.
    */
   function addNewUser($username, $password, $email){
      $ulevel = USER_LEVEL;
      do
      {
        $tmp_id = rand31();
        $result = pg_query($this->connection, "SELECT * FROM users WHERE user_id = $tmp_id" );
      } while ( pg_numrows( $result ) > 0 );

      $q = "INSERT INTO users ( user_id, username, password, email )
        VALUES ( $tmp_id, '$username', '$password', '$email' )";
      return pg_query($this->connection, $q);
   }

   function addNewHost($user_id){
      $result = pg_query($this->connection, "SELECT * FROM hosts WHERE user_id = $user_id" );
      /*if ( pg_numrows( $result ) > 0 )
      {
        return 0;
      }*/

      do
      {
        $tmp_id = -rand31();
        $result = pg_query($this->connection, "SELECT * FROM hosts WHERE host_id = $tmp_id" );
      } while ( pg_numrows( $result ) > 0 );

      $q = "INSERT INTO hosts ( host_id, user_id, online ) VALUES ( $tmp_id, $user_id, -1 )";
      $result = pg_query($this->connection, $q);
      if ( !$result )
      {
        return 0;
      }
      else
      {
        return $tmp_id;
      }
   }


   /**
    * updateUserField - Updates a field, specified by the field
    * parameter, in the user's row of the database.
    */
   function updateUserField($username, $field, $value){
      $q = "UPDATE users SET ".$field." = '$value' WHERE username = '$username'";
      return pg_query($this->connection, $q);
   }

   /**
    * getUserInfo - Returns the result array from a mysql
    * query asking for all information stored regarding
    * the given username. If query fails, NULL is returned.
    */
   function getUserInfo($username){
      $q = "SELECT * FROM users WHERE username = '$username'";
      $result = pg_query($this->connection, $q);
      /* Error occurred, return given name by default */
      if(!$result || (pg_numrows($result) < 1)){
         return NULL;
      }
      /* Return result array */
      $dbarray = pg_fetch_array($result);
      return $dbarray;
   }

   /**
    * query - Performs the given query on the database and
    * returns the result, which may be false, true or a
    * resource identifier.
    */
   function query($query){
      return pg_query( $this->connection, $query);
   }
};

/* Create database connection */
$database = new MyPostgreSQL;

?>
