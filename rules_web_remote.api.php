<?php
// $Id$

/**
 * @file
 * This file contains no working PHP code; it exists to provide additional
 * documentation for doxygen as well as to document remotes in the standard
 * Drupal manner.
 */

/**
 * @addtogroup rules_hooks
 * @{
 */

/**
 * Define a remote endpoint type.
 *
 * This hook may be used to define a remote endpoint type, which users may
 * use for configuring remote sites.
 *
 * @return
 *   An array of endpoint type definitions with the endpoint type names as keys.
 *   Each definition is represented by another array with the following keys:
 *   - label: The label of the endpoint type. Start capitalized. Required.
 *   - class: The actual implementation class for the endpoint type. This class
 *     has to implement the RulesWebRemoteEndpointInterface. Required.
 *
 * @see hook_rules_endpoint_types_alter()
 * @see RulesWebRemoteEndpointInterface
 */
function hook_rules_endpoint_types() {
  return array(
    'rules_web_hook' => array(
      'label' => t('Rules Web Hooks'),
      'class' => 'RulesWebRemoteEndpointWebHooks',
    ),
  );
}

/**
 * Alter remote endpoint type definitions.
 *
 * @param $types
 *   The remote endpoint type definitions as returned from
 *   hook_rules_endpoint_types().
 *
 * @see hook_rules_endpoint_types()
 */
function hook_rules_endpoint_types_alter(&$types) {

}

/**
 * Act on rules web remote sites being loaded from the database.
 *
 * This hook is invoked during rules web remotes loading, which is handled by
 * entity_load(), via the EntityCRUDController.
 *
 * @param $remotes
 *   An array of rules web remote sites being loaded, keyed by id.
 */
function hook_rules_web_remote_load($remotes) {
  $result = db_query('SELECT id, foo FROM {mytable} WHERE id IN(:ids)', array(':ids' => array_keys($remotes)));
  foreach ($result as $record) {
    $remotes[$record->id]->foo = $record->foo;
  }
}

/**
 * Respond to creation of a new rules web remote site.
 *
 * This hook is invoked after the rules web remote is inserted into the
 * database.
 *
 * @param RulesWebRemote $remote
 *   The rules web remote site that is being created.
 */
function hook_rules_web_remote_insert($remote) {
  db_insert('mytable')
    ->fields(array(
      'id' => $remote->id,
      'my_field' => $remote->myField,
    ))
    ->execute();
}

/**
 * Act on a rules web remotes being inserted or updated.
 *
 * This hook is invoked before the rules web remote site is saved to the
 * database.
 *
 * @param RulesWebRemote $remote
 *   The rules web remote site that is being inserted or updated.
 */
function hook_rules_web_remote_presave($remote) {
  $remote->myField = 'foo';
}

/**
 * Respond to updates to a rules web remote.
 *
 * This hook is invoked after the remote site has been updated in the database.
 *
 * @param RulesWebRemote $remote
 *   The rules web remote site that is being updated.
 */
function hook_rules_web_remote_update($remote) {
  db_update('mytable')
    ->fields(array('my_field' => $remote->myField))
    ->condition('id', $remote->id)
    ->execute();
}

/**
 * Respond to a remote site deletion.
 *
 * This hook is invoked after the remote site has been removed from the
 * database.
 *
 * @param RulesWebRemote $remote
 *   The rules web remote site that is being deleted.
 */
function hook_rules_web_remote_delete($remote) {
  db_delete('mytable')
    ->condition('id', $remote->id)
    ->execute();
}

/**
 * Define default rules web remote sites.
 *
 * This hook is invoked when remote sites are loaded.
 *
 * @return
 *   An array of rules web remote sites with the remote site names as keys.
 *
 * @see hook_default_rules_web_remote_alter()
 */
function hook_default_rules_web_remote() {
  $remote = new RulesWebRemote();
  $remote->name = 'master';
  $remote->label = 'The master site.';
  $remote->url = 'http://master.example.com';
  $remote->type = 'rules_web_hook';
  $remotes[$remote->name] = $remote;
  return $remotes;
}

/**
 * Alter default remote sites.
 *
 * @param $remotes
 *   The default remote sites of all modules as returned from
 *   hook_default_rules_web_remote().
 *
 * @see hook_default_rules_web_remote()
 */
function hook_default_rules_web_remote_alter(&$remotes) {

}

/**
 * @}
 */
