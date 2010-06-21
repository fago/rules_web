<?php
// $Id$

/**
 * @file
 * This file contains no working PHP code; it exists to provide additional
 * documentation for doxygen as well as to document hooks in the standard
 * Drupal manner.
 */

/**
 * @addtogroup rules_hooks
 * @{
 */

/**
 * Act on rules web hooks being loaded from the database.
 *
 * This hook is invoked during rules web hooks loading, which is handled by
 * entity_load(), via the EntityCRUDController.
 *
 * @param $hooks
 *   An array of rules web hooks being loaded, keyed by id.
 */
function hook_rules_web_hook_load($hooks) {
  $result = db_query('SELECT id, foo FROM {mytable} WHERE id IN(:ids)', array(':ids' => array_keys($hooks)));
  foreach ($result as $record) {
    $hooks[$record->id]->foo = $record->foo;
  }
}

/**
 * Respond to creation of a new rules web hook.
 *
 * This hook is invoked after the rules web hook is inserted into the database.
 *
 * @param EntityDB $hook
 *   The rules web hook that is being created.
 */
function hook_rules_web_hook_insert($hook) {
  db_insert('mytable')
    ->fields(array(
      'id' => $hook->id,
      'my_field' => $hook->myField,
    ))
    ->execute();
}

/**
 * Act on a rules web hooks being inserted or updated.
 *
 * This hook is invoked before the rules web hook is saved to the database.
 *
 * @param EntityDB $hook
 *   The rules web hook that is being inserted or updated.
 */
function hook_rules_web_hook_presave($hook) {
  $hook->myField = 'foo';
}

/**
 * Respond to updates to a rules web hook.
 *
 * This hook is invoked after the web hook has been updated in the database.
 *
 * @param EntityDB $hook
 *   The rules web hook that is being updated.
 */
function hook_rules_web_hook_update($hook) {
  db_update('mytable')
    ->fields(array('my_field' => $hook->myField))
    ->condition('id', $hook->id)
    ->execute();
}

/**
 * Respond to a rules web hook deletion.
 *
 * This hook is invoked after the web hook has been removed from the database.
 *
 * @param EntityDB $hook
 *   The rules web hook that is being deleted.
 */
function hook_rules_web_hook_delete($hook) {
  db_delete('mytable')
    ->condition('id', $hook->id)
    ->execute();
}

/**
 * Define default rules web hooks.
 *
 * This hook is invoked when rules web hooks are loaded.
 *
 * @return
 *   An array of rules web hooks with the hook names as keys.
 *
 * @see hook_default_rules_web_hook_alter()
 */
function hook_default_rules_web_hook() {
  $hook = new EntityDB(array(), 'rules_web_hook');
  $hook->name = 'test';
  $hook->label = 'A test hook.';
  $hook->active = TRUE;
  $hook->variables = array(
    'node' => array(
      'type' => 'node',
      'label' => 'Content',
    ),
  );
  $hooks[$hook->name] = $hook;
  return $hooks;
}

/**
 * Alter default web hooks.
 *
 * @param $hooks
 *   The default hooks of all modules as returned from
 *   hook_default_rules_web_hook().
 *
 * @see hook_default_rules_web_hook()
 */
function hook_default_rules_web_hook_alter(&$hooks) {

}

/**
 * @}
 */
