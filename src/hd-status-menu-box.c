/*
 * This file is part of hildon-status-menu
 * 
 * Copyright (C) 2008 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "hd-status-menu-box.h"

/* UI Style guide */
#define ITEM_HEIGHT 70
#define ITEM_WIDTH 328

struct _HDStatusMenuBoxPrivate
{
  GList *children;

  guint visible_items;
  guint columns;
};


typedef struct _HDStatusMenuBoxChild HDStatusMenuBoxChild;
struct _HDStatusMenuBoxChild
{
  GtkWidget *widget;
  guint      priority;
};

enum
{
  PROP_0,
  PROP_VISIBLE_ITEMS,
  PROP_COLUMNS
};
 
G_DEFINE_TYPE (HDStatusMenuBox, hd_status_menu_box, GTK_TYPE_CONTAINER);

static void
hd_status_menu_box_get_property (GObject      *object,
                                 guint         prop_id,
                                 GValue       *value,
                                 GParamSpec   *pspec)
{
  HDStatusMenuBoxPrivate *priv = HD_STATUS_MENU_BOX (object)->priv;

  switch (prop_id)
    {
    case PROP_VISIBLE_ITEMS:
      g_value_set_uint (value, priv->visible_items);
      break;

    case PROP_COLUMNS:
      g_value_set_uint (value, priv->columns);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
hd_status_menu_box_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  HDStatusMenuBoxPrivate *priv = HD_STATUS_MENU_BOX (object)->priv;

  switch (prop_id)
    {
    case PROP_COLUMNS:
      priv->columns = g_value_get_uint (value);
      gtk_widget_queue_resize (GTK_WIDGET (object));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gint
hd_status_menu_box_cmp_priority (gconstpointer a,
                                 gconstpointer b)
{
  if (((HDStatusMenuBoxChild *)a)->priority >
      ((HDStatusMenuBoxChild *)b)->priority)
    return 1;

  return -1;
}

static void
hd_status_menu_box_add (GtkContainer *container,
                        GtkWidget    *child)
{
  hd_status_menu_box_pack (HD_STATUS_MENU_BOX (container), child, G_MAXUINT);
}

static void
hd_status_menu_box_remove (GtkContainer *container,
                           GtkWidget    *child)
{
  HDStatusMenuBoxPrivate *priv;
  GList *c;

  g_return_if_fail (HD_IS_STATUS_MENU_BOX (container));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == (GtkWidget *)container);

  priv = HD_STATUS_MENU_BOX (container)->priv;

  /* search for child in children and remove it */
  for (c = priv->children; c; c = c->next)
    {
      HDStatusMenuBoxChild *info = c->data;

      if (info->widget == child)
        {
          gboolean visible;

          visible = gtk_widget_is_visible (child);

          gtk_widget_unparent (child);

          priv->children = g_list_delete_link (priv->children, c);
          g_slice_free (HDStatusMenuBoxChild, info);

          /* resize container if child was visible */
          if (visible)
            gtk_widget_queue_resize (GTK_WIDGET (container));

          break;
        }
    }
}

static void
hd_status_menu_box_forall (GtkContainer *container,
                           gboolean      include_internals,
                           GtkCallback   callback,
                           gpointer      data)
{
  HDStatusMenuBoxPrivate *priv;
  GList *c;

  g_return_if_fail (HD_IS_STATUS_MENU_BOX (container));

  priv = HD_STATUS_MENU_BOX (container)->priv;

  for (c = priv->children; c; )
    {
      HDStatusMenuBoxChild *info = c->data;

      /* callback could destroy c */
      c = c->next;

      (* callback) (info->widget, data);
    }
}

static GType
hd_status_menu_box_child_type (GtkContainer *container)
{
  return GTK_TYPE_WIDGET;
}

static void
hd_status_menu_box_get_child_property (GtkContainer *container,
                                       GtkWidget    *child,
                                       guint         propid,
                                       GValue       *value,
                                       GParamSpec   *pspec)
{
}

static void
hd_status_menu_box_set_child_property (GtkContainer *container,
                                       GtkWidget    *child,
                                       guint         propid,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
}

static void
hd_status_menu_box_size_allocate (GtkWidget     *widget,
                                  GtkAllocation *allocation)
{
  HDStatusMenuBoxPrivate *priv;
  guint border_width;
  GtkAllocation child_allocation = {0, 0, 0, 0};
  guint visible_children = 0;
  GList *c;

  priv = HD_STATUS_MENU_BOX (widget)->priv;

  border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

  /* chain up */
  GTK_WIDGET_CLASS (hd_status_menu_box_parent_class)->size_allocate (widget,
                                                                     allocation);

  child_allocation.width = (allocation->width - (2 * border_width)) / priv->columns;
  child_allocation.height = ITEM_HEIGHT;

  /* place the visible children */
  for (c = priv->children; c; c = c->next)
    {
      HDStatusMenuBoxChild *info = c->data;

      /* ignore hidden widgets */
      if (!gtk_widget_is_visible (info->widget))
        continue;

      child_allocation.x = allocation->x + border_width + (visible_children % priv->columns * child_allocation.width);
      child_allocation.y = allocation->y + border_width + (visible_children / priv->columns * ITEM_HEIGHT);

      gtk_widget_size_allocate (info->widget, &child_allocation);

      visible_children++;
    }
}

static void
hd_status_menu_box_size_request (GtkWidget      *widget,
                                 GtkRequisition *requisition)
{
  HDStatusMenuBoxPrivate *priv;
  guint border_width;
  GList *c;
  guint visible_children = 0;

  priv = HD_STATUS_MENU_BOX (widget)->priv;

  border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

  /* calculate number of visible children */
  for (c = priv->children; c; c = c->next)
    {
      HDStatusMenuBoxChild *info = c->data;
      GtkRequisition child_requisition;

      if (!gtk_widget_is_visible (info->widget))
        continue;

      visible_children++;

      /* there are some widgets which need a size request */
      gtk_widget_get_preferred_size (info->widget, &child_requisition, NULL);
    }

  /* Update ::visible-items property if required */
  if (priv->visible_items != visible_children)
    {
      priv->visible_items = visible_children;
      g_object_notify (G_OBJECT (widget), "visible-items");
    }

  /* width is always two columns */
  requisition->width = 10; // 2 * ITEM_WIDTH + 2 * border_width;
  /* height is at least one row */
  requisition->height = MAX ((visible_children + (priv->columns - 1)) / priv->columns, 1) * ITEM_HEIGHT + 2 * border_width;
}

static void
hd_status_menu_box_get_preferred_width (GtkWidget *widget,
                                        gint      *minimal_width,
                                        gint      *natural_width)
{
  GtkRequisition requisition;

  hd_status_menu_box_size_request (widget, &requisition);

  *minimal_width = *natural_width = requisition.width;
}

static void
hd_status_menu_box_get_preferred_height (GtkWidget *widget,
                                         gint      *minimal_height,
                                         gint      *natural_height)
{
  GtkRequisition requisition;

  hd_status_menu_box_size_request (widget, &requisition);

  *minimal_height = *natural_height = requisition.height;
}

static void
hd_status_menu_box_class_init (HDStatusMenuBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = hd_status_menu_box_get_property;
  object_class->set_property = hd_status_menu_box_set_property;

  container_class->add = hd_status_menu_box_add;
  container_class->remove = hd_status_menu_box_remove;
  container_class->forall = hd_status_menu_box_forall;
  container_class->child_type = hd_status_menu_box_child_type;
  container_class->get_child_property = hd_status_menu_box_get_child_property;
  container_class->set_child_property = hd_status_menu_box_set_child_property;

  widget_class->size_allocate = hd_status_menu_box_size_allocate;
  widget_class->get_preferred_width = hd_status_menu_box_get_preferred_width;
  widget_class->get_preferred_height = hd_status_menu_box_get_preferred_height;

  g_object_class_install_property (object_class,
                                   PROP_VISIBLE_ITEMS,
                                   g_param_spec_uint ("visible-items",
                                                      "Visible Items",
                                                      "Number of visible items",
                                                      0,
                                                      G_MAXUINT,
                                                      0,
                                                      G_PARAM_READABLE));
  g_object_class_install_property (object_class,
                                   PROP_COLUMNS,
                                   g_param_spec_uint ("columns",
                                                      "Columns",
                                                      "Number of columns",
                                                      1,
                                                      G_MAXUINT,
                                                      2,
                                                      G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

  g_type_class_add_private (klass, sizeof (HDStatusMenuBoxPrivate));
}

static void
hd_status_menu_box_init (HDStatusMenuBox *box)
{
  gtk_widget_set_has_window (GTK_WIDGET (box), FALSE);
  gtk_widget_set_redraw_on_allocate (GTK_WIDGET (box),
                                     TRUE);

  box->priv = G_TYPE_INSTANCE_GET_PRIVATE ((box), HD_TYPE_STATUS_MENU_BOX, HDStatusMenuBoxPrivate);

  box->priv->children = NULL;

  box->priv->columns = 2;
}

GtkWidget *
hd_status_menu_box_new (void)
{
  GtkWidget *box;

  box = g_object_new (HD_TYPE_STATUS_MENU_BOX,
                      NULL);

  return box;
}

void
hd_status_menu_box_pack (HDStatusMenuBox *box,
                         GtkWidget       *child,
                         guint            position)
{
  HDStatusMenuBoxPrivate *priv;
  HDStatusMenuBoxChild *info;

  g_return_if_fail (HD_IS_STATUS_MENU_BOX (box));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == NULL);

  priv = box->priv;

  info = g_slice_new0 (HDStatusMenuBoxChild);
  info->widget = child;
  info->priority = position;

  priv->children = g_list_insert_sorted (priv->children,
                                         info,
                                         hd_status_menu_box_cmp_priority);

  gtk_widget_set_parent (child, GTK_WIDGET (box));
}

void
hd_status_menu_box_reorder_child (HDStatusMenuBox *box,
                                  GtkWidget       *child,
                                  guint            position)
{
  HDStatusMenuBoxPrivate *priv;
  GList *c;

  g_return_if_fail (HD_IS_STATUS_MENU_BOX (box));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == GTK_WIDGET (box));

  priv = box->priv;

  for (c = priv->children; c; c = c->next)
    {
      HDStatusMenuBoxChild *info = c->data;

      if (info->widget == child)
        {
          if (info->priority != position)
            {
              info->priority = position;

              /* Reorder children list */
              priv->children = g_list_delete_link (priv->children, c);
              priv->children = g_list_insert_sorted (priv->children,
                                                     info,
                                                     hd_status_menu_box_cmp_priority);
            }

          break;
        }
    }
}
